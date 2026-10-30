/* What the engine and UI call: start/stop the sound worker, say what is wanted, read its state. */
#include "audio_internal.h"

#include "util/util.h"

#include <string.h>

#define g g_audio

void wye_audio_start(void)
{
    g.lock = plat_mutex_new();
    g.wake = plat_event_new();
    g.stop = false;
    g.thread = plat_thread_start(audio_worker, NULL);
}

void wye_audio_stop(void)
{
    if (!g.thread) {
        return;
    }
    g.stop = true;
    plat_event_signal(g.wake);
    plat_thread_join(g.thread);
    g.thread = NULL;
}

void wye_audio_want(const char *bridge, bool speaker, bool mic)
{
    if (!g.lock) {
        return;
    }
    plat_mutex_lock(g.lock);
    bool changed = strcmp(g.bridge, bridge) || g.want[0] != speaker || g.want[1] != mic;
    wye_strcpy(g.bridge, sizeof(g.bridge), bridge);
    g.want[WYE_ROUTE_SPEAKER] = speaker;
    g.want[WYE_ROUTE_MIC] = mic;
    plat_mutex_unlock(g.lock);
    if (changed) {
        plat_event_signal(g.wake); /* act on switches at once, not at the next check */
    }
}

void wye_audio_status(struct wye_audio_status *out)
{
    memset(out, 0, sizeof(*out));
    if (!g.lock) {
        return;
    }
    plat_mutex_lock(g.lock);
    *out = g.st;
    plat_mutex_unlock(g.lock);
    for (int i = 0; i < WYE_ROUTE_COUNT; ++i) {
        out->route[i].level = g.route[i].peak;
    }
}
