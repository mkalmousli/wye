/* Routine commands (UI thread; the engine lock is taken here). */
#include "engine_internal.h"

#include "util/util.h"

bool wye_engine_record(bool start, const char *name)
{
    struct wye_engine *e = &g_engine;
    bool ok = true;
    plat_mutex_lock(e->lock);
    if (start && !e->player.active) {
        ok = wye_recorder_start(&e->recorder, name, plat_now_ms());
    } else if (!start) {
        wye_recorder_stop(&e->recorder);
    } else {
        ok = false;
    }
    plat_mutex_unlock(e->lock);
    return ok;
}

bool wye_engine_play(const char *path, bool loop, char *err, size_t errlen)
{
    struct wye_engine *e = &g_engine;
    plat_mutex_lock(e->lock);
    bool ok = !e->recorder.file;
    if (!ok) {
        wye_strcpy(err, errlen, "Stop recording first");
    } else if ((ok = wye_player_load(&e->player, path, err, errlen))) {
        wye_player_start(&e->player, plat_now_ms(), loop);
    }
    plat_mutex_unlock(e->lock);
    plat_event_signal(e->wake);
    return ok;
}

void wye_engine_play_stop(void)
{
    plat_mutex_lock(g_engine.lock);
    wye_player_stop(&g_engine.player);
    wye_link_send(&g_engine.link, WYE_NODE_TARGET, WYE_MSG_RELEASE_ALL, NULL, 0);
    plat_mutex_unlock(g_engine.lock);
}

void wye_engine_play_pause(void)
{
    plat_mutex_lock(g_engine.lock);
    wye_player_pause(&g_engine.player, plat_now_ms());
    plat_mutex_unlock(g_engine.lock);
}

void wye_engine_play_speed(int percent)
{
    struct wye_player *p = &g_engine.player;
    plat_mutex_lock(g_engine.lock);
    uint64_t now = plat_now_ms();
    uint64_t elapsed = wye_player_elapsed(p, now);
    p->speed_percent = percent < 25 ? 25 : (percent > 1000 ? 1000 : percent);
    /* Re-anchor so the playhead does not jump when the speed changes. */
    if (p->active) {
        p->start = (p->paused ? p->paused_at : now) - p->paused_total -
                   elapsed * 100u / (uint64_t)p->speed_percent;
    }
    plat_mutex_unlock(g_engine.lock);
}

int wye_engine_recording_tail(struct wye_routine_event *out, int max)
{
    plat_mutex_lock(g_engine.lock);
    int n = wye_recorder_tail(&g_engine.recorder, out, max);
    plat_mutex_unlock(g_engine.lock);
    return n;
}

bool wye_engine_last_recording(char *out, size_t len)
{
    plat_mutex_lock(g_engine.lock);
    bool ok = g_engine.recorder.path[0] && wye_strcpy(out, len, g_engine.recorder.path);
    plat_mutex_unlock(g_engine.lock);
    return ok;
}
