/* Background update checks and (auto-)installation. */
#include "update.h"

#include "log.h"
#include "update_internal.h"
#include "product.h"
#include "util/util.h"
#include "util/version.h"

#include <string.h>

#define FIRST_CHECK_DELAY_MS 10000u

struct wye_updater g_up;

static void set_state(enum wye_update_state s, const char *err)
{
    plat_mutex_lock(g_up.lock);
    g_up.st.state = s;
    wye_strcpy(g_up.st.error, sizeof(g_up.st.error), err ? err : "");
    plat_mutex_unlock(g_up.lock);
}

static void do_check(void)
{
    struct wye_update_manifest m;
    char err[160] = "", url[WYE_CFG_STR];
    plat_mutex_lock(g_up.lock);
    wye_strcpy(url, sizeof(url), g_up.cfg.update_url);
    plat_mutex_unlock(g_up.lock);
    set_state(WYE_UPD_CHECKING, NULL);
    bool ok = wye_update_fetch_manifest(url, &m, err, sizeof(err));
    plat_mutex_lock(g_up.lock);
    g_up.st.last_check = plat_unix_time();
    if (ok) {
        g_up.st.manifest = m;
    }
    plat_mutex_unlock(g_up.lock);
    if (!ok) {
        LOG_W("update: %s", err);
        set_state(WYE_UPD_ERROR, err);
        return;
    }
    bool newer = wye_version_compare(m.version, WYE_VERSION_STRING) > 0;
    LOG_I("update: latest %s, running %s", m.version, WYE_VERSION_STRING);
    set_state(newer ? WYE_UPD_AVAILABLE : WYE_UPD_CURRENT, NULL);
    g_up.install_req = g_up.install_req || (newer && g_up.cfg.update_mode == WYE_UPDATE_AUTO);
}

static void do_install(void)
{
    char err[160] = "";
    struct wye_update_manifest m;
    plat_mutex_lock(g_up.lock);
    m = g_up.st.manifest;
    plat_mutex_unlock(g_up.lock);
    set_state(WYE_UPD_DOWNLOADING, NULL);
    bool ok = wye_update_install(&m, err, sizeof(err));
    set_state(ok ? WYE_UPD_RESTARTING : WYE_UPD_ERROR, ok ? NULL : err);
}

void wye_updater_service(void *arg)
{
    (void)arg;
    uint64_t next = plat_now_ms() + FIRST_CHECK_DELAY_MS;
    while (!g_up.stop && g_up.st.state != WYE_UPD_RESTARTING) {
        plat_event_wait(g_up.wake, 1000);
        bool scheduled = g_up.cfg.update_mode != WYE_UPDATE_OFF && plat_now_ms() >= next;
        if (g_up.check_req || scheduled) {
            g_up.check_req = false;
            next = plat_now_ms() + WYE_UPDATE_CHECK_INTERVAL_S * 1000ull;
            do_check();
        }
        if (g_up.install_req && !g_up.busy && g_up.st.state == WYE_UPD_AVAILABLE) {
            g_up.install_req = false;
            do_install();
        }
    }
}
