/*
 * Wye: one binary for everything. Normal launches open the GUI; the
 * flags below are used internally (privileged helper, updater relaunch,
 * release signing in CI) and never require a terminal from users.
 */
#include "app_entry.h"
#include "ui.h"

#include "api/api.h"
#include "config/config.h"
#include "engine/engine.h"
#include "log.h"
#include "plat_sys.h"
#include "plat_thread.h"
#include "product.h"
#include "update/update.h"

#include <stdlib.h>
#include <string.h>

#define EXIT_DEADLINE_MS 10000

static bool has_flag(int argc, char **argv, const char *flag)
{
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], flag) == 0) {
            return true;
        }
    }
    return false;
}

static bool acquire_instance(bool updated)
{
    /* After a self-update the old process may still be exiting: wait for it. */
    for (int i = 0; i < (updated ? 50 : 1); ++i) {
        if (plat_instance_acquire(!updated)) {
            return true;
        }
        plat_sleep_ms(200);
    }
    return false;
}

/* Closing means gone: a stuck thread, like a hanging download, must not keep the process alive. */
static void exit_deadline(void *arg)
{
    (void)arg;
    plat_sleep_ms(EXIT_DEADLINE_MS);
    _Exit(0);
}

static void init_config(struct wye_ui *ui)
{
    if (!wye_config_load(&ui->cfg) || ui->cfg.first_run == 0) {
        ui->page = PAGE_SETUP; /* first launch: guide the user */
    }
    if (ui->cfg.first_run == 0) {
        ui->cfg.first_run = plat_unix_time();
        wye_config_save(&ui->cfg);
    }
}

int wye_main(int argc, char **argv)
{
    int special = wye_special_mode(argc, argv);
    if (special >= 0) {
        return special;
    }
    if (!has_flag(argc, argv, "--foreground")) {
        plat_detach_from_terminal(); /* before the lock: its PID must be the one that stays */
    }
    bool updated = has_flag(argc, argv, "--updated");
    if (!acquire_instance(updated)) {
        return 0; /* the running instance was asked to show itself */
    }
    plat_cleanup_previous_version();
    static struct wye_ui ui;
    char dir[PLAT_PATH_MAX];
    wye_log_init(plat_data_dir(dir, sizeof(dir)) ? dir : NULL);
    char os[160];
    plat_os_description(os, sizeof(os));
    LOG_I("%s %s starting on %s", WYE_PRODUCT_NAME, WYE_VERSION_STRING, os);
    init_config(&ui);
    ui.hidden = ui.cfg.start_hidden || has_flag(argc, argv, "--background");
    wye_engine_start(&ui.cfg);
    if (ui.cfg.api_enabled) {
        wye_api_start(wye_engine_api_ops());
    }
    wye_updater_start(&ui.cfg);
    if (updated) {
        wye_engine_notice("%s was updated to %s", WYE_PRODUCT_NAME, WYE_VERSION_STRING);
    }
    int rc = wye_ui_run(&ui);
    plat_thread_start(exit_deadline, NULL);
    wye_updater_stop();
    wye_api_stop();
    wye_engine_stop();
    LOG_I("exit");
    wye_log_shutdown();
    return rc;
}
