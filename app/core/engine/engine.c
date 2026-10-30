/* Engine lifecycle and the I/O thread. */
#include "engine_internal.h"

#include "log.h"
#include "util/util.h"

#include <string.h>

struct wye_engine g_engine;

static int wait_budget(struct wye_engine *e)
{
    bool busy = e->input.mode != WYE_FWD_OFF || e->player.active || wye_typing_busy(&e->typing) ||
                e->jiggler.active || wye_fw_session_busy(&e->fw);
    return busy ? 2 : 20;
}

static void engine_thread(void *arg)
{
    struct wye_engine *e = arg;
    uint8_t buf[4096];
    while (!e->stop) {
        /* Only this thread opens/closes the port, so reading it unlocked is safe. */
        plat_serial *port = e->link.port;
        int n = 0;
        if (port) {
            n = plat_serial_read(port, buf, sizeof(buf), wait_budget(e), e->wake);
        } else {
            plat_event_wait(e->wake, wait_budget(e));
        }
        plat_mutex_lock(e->lock);
        if (n < 0 && e->link.port == port) {
            wye_link_close(&e->link, "device disconnected");
        }
        wye_link_process(&e->link, buf, n > 0 ? (size_t)n : 0);
        wye_engine_tick(e, plat_now_ms());
        plat_mutex_unlock(e->lock);
        wye_engine_capture_watch(e, plat_now_ms());
    }
}

bool wye_engine_start(const struct wye_config *cfg)
{
    struct wye_engine *e = &g_engine;
    memset(e, 0, sizeof(*e));
    e->lock = plat_mutex_new();
    e->wake = plat_event_new();
    e->cfg = *cfg;
    wye_engine_transport_init(e);
    wye_engine_input_init(e);
    wye_drive_init(&e->drive, cfg->drive_enabled, cfg->drive_folder, (uint32_t)cfg->drive_size_mb);
    wye_audio_start();
    wye_engine_apply_config(cfg);
    wye_engine_capture_start(e);
    e->thread = plat_thread_start(engine_thread, e);
    return e->thread != NULL;
}

void wye_engine_stop(void)
{
    struct wye_engine *e = &g_engine;
    if (!e->thread) {
        return;
    }
    plat_mutex_lock(e->lock);
    wye_input_set_mode(&e->input, WYE_FWD_OFF);
    wye_recorder_stop(&e->recorder);
    plat_mutex_unlock(e->lock);
    plat_input_stop();
    e->stop = true;
    plat_event_signal(e->wake);
    plat_thread_join(e->thread);
    e->thread = NULL;
    wye_link_close(&e->link, "shutting down");
    wye_serial_worker_free(e->serial_worker);
    e->serial_worker = NULL;
    wye_player_stop(&e->player);
    wye_fw_session_reset(&e->fw);
    wye_drive_shutdown(&e->drive);
    wye_audio_stop();
}
