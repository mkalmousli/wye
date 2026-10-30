/* Routine playback with pause, loop and speed control. */
#include "routine.h"

#include "util/util.h"
#include "wye_proto.h"

#include <stdlib.h>
#include <string.h>

#define ROUTINE_FILE_MAX (64u * 1024u * 1024u)

bool wye_player_load(struct wye_player *p, const char *path, char *err, size_t errlen)
{
    wye_player_stop(p);
    size_t size = 0;
    char *data = (char *)wye_read_file(path, &size, ROUTINE_FILE_MAX);
    if (!data) {
        wye_strcpy(err, errlen, "cannot read routine file");
        return false;
    }
    size_t cap = size / 8 + 1;
    p->events = calloc(cap, sizeof(*p->events));
    for (char *line = strtok(data, "\n"); line && p->events && p->count < cap; line = strtok(NULL, "\n")) {
        if (wye_routine_parse_line(line, &p->events[p->count])) {
            p->count++;
        }
    }
    free(data);
    if (p->count == 0) {
        wye_player_stop(p);
        wye_strcpy(err, errlen, "routine has no events");
        return false;
    }
    const char *base = strrchr(path, '/');
    const char *base2 = strrchr(path, '\\');
    base = base2 > base ? base2 : base;
    wye_strcpy(p->name, sizeof(p->name), base ? base + 1 : path);
    return true;
}

void wye_player_start(struct wye_player *p, uint64_t now, bool loop)
{
    p->active = p->count > 0;
    p->paused = false;
    p->loop = loop;
    p->speed_percent = p->speed_percent > 0 ? p->speed_percent : 100;
    p->index = 0;
    p->start = now;
    p->paused_total = 0;
}

uint64_t wye_player_elapsed(const struct wye_player *p, uint64_t now)
{
    uint64_t at = p->paused ? p->paused_at : now;
    uint64_t real = at - p->start - p->paused_total;
    return real * (uint64_t)p->speed_percent / 100u;
}

void wye_player_tick(struct wye_player *p, uint64_t now, const struct wye_sender *out)
{
    if (!p->active || p->paused) {
        return;
    }
    uint64_t elapsed = wye_player_elapsed(p, now);
    while (p->index < p->count && p->events[p->index].t <= elapsed) {
        const struct wye_routine_event *ev = &p->events[p->index++];
        if (!wye_send(out, ev->type, ev->payload, ev->len)) {
            wye_player_stop(p);
            return;
        }
    }
    if (p->index >= p->count) {
        wye_send(out, WYE_MSG_RELEASE_ALL, NULL, 0); /* an edit may have removed a key's release */
        if (p->loop) {
            wye_player_start(p, now, true);
        } else {
            p->active = false;
        }
    }
}

void wye_player_pause(struct wye_player *p, uint64_t now)
{
    if (!p->active) {
        return;
    }
    if (p->paused) {
        p->paused_total += now - p->paused_at;
    } else {
        p->paused_at = now;
    }
    p->paused = !p->paused;
}

void wye_player_stop(struct wye_player *p)
{
    int speed = p->speed_percent;
    free(p->events);
    memset(p, 0, sizeof(*p));
    p->speed_percent = speed;
}

int wye_player_progress(const struct wye_player *p, uint64_t now)
{
    if (!p->active || p->count == 0 || p->events[p->count - 1].t == 0) {
        return 0;
    }
    uint64_t pct = wye_player_elapsed(p, now) * 100u / p->events[p->count - 1].t;
    return pct > 100 ? 100 : (int)pct;
}
