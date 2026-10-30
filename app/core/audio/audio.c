/* The sound worker: keeps the speaker and microphone routes as wanted, re-opening them when devices change. */
#include "audio_internal.h"

#include "log.h"
#include "plat_sys.h"
#include "plat_thread.h"
#include "util/util.h"

#include <string.h>

#define CHECK_MS 1500u /* devices are looked at again this often */

struct audio_shared g_audio;
#define g g_audio

/* What a route should connect now; false (with a reason) if it cannot. */
static bool plan(ma_context *ctx, int i, const char *bridge, struct wye_route_status *s)
{
    struct wye_audio_pick pico, local;
    bool speaker = i == WYE_ROUTE_SPEAKER;
    audio_pick(ctx, bridge, speaker, &pico, &local); /* speaker: record the Pico; mic: play to it */
    if (!pico.found || !local.found) {
        wye_strcpy(s->problem, sizeof(s->problem), !pico.found ? "Waiting for the bridge's sound card"
                   : speaker ? "No speakers on this computer" : "No microphone on this computer");
        return false;
    }
    const struct wye_audio_pick *from = speaker ? &pico : &local, *to = speaker ? &local : &pico;
    wye_strcpy(s->from, sizeof(s->from), from->name);
    wye_strcpy(s->to, sizeof(s->to), to->name);
    g.from[i] = from->id;
    g.to[i] = to->id;
    return true;
}

static void reconcile(ma_context *ctx)
{
    char bridge[64];
    bool want[WYE_ROUTE_COUNT];
    plat_mutex_lock(g.lock);
    wye_strcpy(bridge, sizeof(bridge), g.bridge);
    memcpy(want, g.want, sizeof(want));
    plat_mutex_unlock(g.lock);
    for (int i = 0; i < WYE_ROUTE_COUNT; ++i) {
        struct wye_route_status s = {.wanted = want[i] && bridge[0]};
        ma_device_id was_from = g.from[i], was_to = g.to[i];
        bool ok = s.wanted && plan(ctx, i, bridge, &s);
        bool moved = memcmp(&was_from, &g.from[i], sizeof(was_from)) || memcmp(&was_to, &g.to[i], sizeof(was_to));
        if (g.route[i].open && (!ok || moved || !audio_route_alive(&g.route[i]))) {
            audio_route_close(&g.route[i]);
        }
        if (ok && !g.route[i].open && audio_route_open(&g.route[i], ctx, &g.from[i], &g.to[i], s.problem,
                                                         sizeof(s.problem))) {
            LOG_I("audio: %s -> %s", s.from, s.to);
            if (ctx->backend == ma_backend_pulseaudio) {
                bool speaker = i == WYE_ROUTE_SPEAKER; /* the Pico's side of the route */
                plat_audio_full_volume(speaker ? g.from[i].pulse : g.to[i].pulse, speaker);
            }
        }
        s.running = g.route[i].open;
        plat_mutex_lock(g.lock);
        g.st.route[i] = s;
        plat_mutex_unlock(g.lock);
    }
}

void audio_worker(void *arg)
{
    (void)arg;
    ma_context ctx;
    if (ma_context_init(NULL, 0, NULL, &ctx) != MA_SUCCESS) {
        LOG_W("audio: no sound system available");
        return;
    }
    while (!g.stop) {
        reconcile(&ctx);
        plat_event_wait(g.wake, CHECK_MS);
    }
    for (int i = 0; i < WYE_ROUTE_COUNT; ++i) {
        audio_route_close(&g.route[i]);
    }
    ma_context_uninit(&ctx);
}
