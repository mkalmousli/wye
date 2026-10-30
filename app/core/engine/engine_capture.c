/* Keyboard/mouse capture lifecycle. */
#include "engine_internal.h"

#include "log.h"
#include "plat_sys.h"

#include <stdlib.h>

void wye_engine_capture_start(struct wye_engine *e)
{
    char err[256] = "";
    if (getenv("WYE_NO_CAPTURE")) { /* tests, screenshots, CI */
        LOG_W("input capture disabled by WYE_NO_CAPTURE");
        e->next_capture_retry = UINT64_MAX;
    } else if (!plat_input_start(wye_engine_sink, e, err, sizeof(err))) {
        LOG_E("input capture unavailable: %s", err);
        wye_engine_notice("Keyboard/mouse capture unavailable: %s", err);
    }
}

/* Starts capture once permission exists. Runs unlocked: stopping capture
   joins a thread that may be waiting for the engine lock inside the sink. */
void wye_engine_capture_watch(struct wye_engine *e, uint64_t now)
{
    enum plat_input_status s = plat_input_status();
    if (s == PLAT_INPUT_RUNNING || now < e->next_capture_retry) {
        return;
    }
    e->next_capture_retry = now + 3000;
    if (plat_input_permission() != PLAT_PERM_NEEDED) {
        char err[160];
        plat_input_stop();
        if (plat_input_start(wye_engine_sink, e, err, sizeof(err))) {
            wye_engine_notice("Keyboard and mouse capture is active");
        }
    }
}
