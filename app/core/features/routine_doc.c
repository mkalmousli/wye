/* Editing a routine in memory: load, change, tidy up, save. */
#include "routine.h"

#include "util/util.h"
#include "wye_codec.h"
#include "wye_msg.h"
#include "wye_proto.h"

#include <stdlib.h>
#include <string.h>

#define ROUTINE_FILE_MAX (64u * 1024u * 1024u)
#define LINE_MAX_BYTES 64u

bool wye_routine_doc_load(struct wye_routine_doc *d, const char *path, char *err, size_t errlen)
{
    wye_routine_doc_free(d);
    size_t size = 0;
    char *data = (char *)wye_read_file(path, &size, ROUTINE_FILE_MAX);
    if (!data) {
        wye_strcpy(err, errlen, "Cannot read this routine");
        return false;
    }
    d->cap = size / 8 + 1;
    d->ev = calloc(d->cap, sizeof(*d->ev));
    for (char *line = strtok(data, "\n"); line && d->ev && d->count < d->cap; line = strtok(NULL, "\n")) {
        if (wye_routine_parse_line(line, &d->ev[d->count])) {
            d->count++;
        }
    }
    free(data);
    if (!d->ev) {
        wye_routine_doc_free(d);
        wye_strcpy(err, errlen, "Not enough memory");
        return false;
    }
    return true;
}

bool wye_routine_doc_save(const struct wye_routine_doc *d, const char *path)
{
    size_t cap = 32 + d->count * LINE_MAX_BYTES, used = 0;
    char *buf = malloc(cap);
    if (!buf) {
        return false;
    }
    used += (size_t)snprintf(buf, cap, "# wye-routine 2\n");
    for (size_t i = 0; i < d->count; ++i) {
        char hex[2 * 16 + 1];
        const struct wye_routine_event *e = &d->ev[i];
        wye_hex_encode(e->payload, e->len, hex, sizeof(hex));
        used += (size_t)snprintf(buf + used, cap - used, "%lu %u %s\n", (unsigned long)e->t, e->type, hex);
    }
    bool ok = wye_write_file_atomic(path, buf, used);
    free(buf);
    return ok;
}

void wye_routine_doc_free(struct wye_routine_doc *d)
{
    free(d->ev);
    memset(d, 0, sizeof(*d));
}

void wye_routine_doc_delete(struct wye_routine_doc *d, size_t from, size_t to)
{
    if (from > to || to >= d->count) {
        return;
    }
    size_t after = to + 1;
    if (after < d->count) {
        uint32_t back = d->ev[after].t - d->ev[from].t;
        for (size_t i = after; i < d->count; ++i) {
            d->ev[i].t -= back;
        }
    }
    memmove(&d->ev[from], &d->ev[after], (d->count - after) * sizeof(d->ev[0]));
    d->count -= after - from;
}

static bool reserve(struct wye_routine_doc *d, size_t want)
{
    if (want <= d->cap) {
        return true;
    }
    size_t cap = want * 2;
    struct wye_routine_event *ev = realloc(d->ev, cap * sizeof(*ev));
    if (!ev) {
        return false;
    }
    d->ev = ev;
    d->cap = cap;
    return true;
}

bool wye_routine_doc_duplicate(struct wye_routine_doc *d, size_t from, size_t to)
{
    if (from > to || to >= d->count) {
        return false;
    }
    size_t n = to - from + 1;
    if (!reserve(d, d->count + n)) {
        return false;
    }
    uint32_t shift = d->ev[to].t - d->ev[from].t + 1; /* the copy starts a moment after the last original */
    memmove(&d->ev[to + 1 + n], &d->ev[to + 1], (d->count - to - 1) * sizeof(d->ev[0]));
    memcpy(&d->ev[to + 1], &d->ev[from], n * sizeof(d->ev[0]));
    d->count += n;
    for (size_t i = to + 1; i < d->count; ++i) {
        d->ev[i].t += shift;
    }
    return true;
}

bool wye_routine_doc_move(struct wye_routine_doc *d, size_t i, int dir)
{
    if (i >= d->count || (dir < 0 && i == 0) || (dir > 0 && i + 1 >= d->count)) {
        return false;
    }
    size_t j = dir < 0 ? i - 1 : i + 1;
    struct wye_routine_event tmp = d->ev[i];
    uint32_t ti = d->ev[i].t, tj = d->ev[j].t;
    d->ev[i] = d->ev[j];
    d->ev[j] = tmp;
    d->ev[i].t = ti;
    d->ev[j].t = tj;
    return true;
}

uint32_t wye_routine_doc_delay(const struct wye_routine_doc *d, size_t i)
{
    return i >= d->count ? 0 : i == 0 ? d->ev[0].t : d->ev[i].t - d->ev[i - 1].t;
}

void wye_routine_doc_set_delay(struct wye_routine_doc *d, size_t i, uint32_t ms)
{
    if (i >= d->count) {
        return;
    }
    uint32_t at = i == 0 ? ms : d->ev[i - 1].t + ms, old = d->ev[i].t;
    for (size_t k = i; k < d->count; ++k) {
        d->ev[k].t = d->ev[k].t - old + at;
    }
}

void wye_routine_doc_speed(struct wye_routine_doc *d, int percent)
{
    if (percent <= 0) {
        return;
    }
    for (size_t i = 0; i < d->count; ++i) {
        d->ev[i].t = (uint32_t)((uint64_t)d->ev[i].t * 100u / (uint64_t)percent);
    }
}

bool wye_routine_is_move(const struct wye_routine_event *ev)
{
    if (ev->type == WYE_MSG_ABS_MOUSE) {
        return true;
    }
    struct wye_mouse m;
    return ev->type == WYE_MSG_MOUSE && wye_mouse_unpack(ev->payload, ev->len, &m) && (m.dx || m.dy) && !m.wheel && !m.pan;
}

/* Adds b's motion to a if both are plain moves with the same buttons, and the sum still fits. */
static bool merge_rel(struct wye_routine_event *a, const struct wye_routine_event *b)
{
    struct wye_mouse ma, mb;
    if (a->type != WYE_MSG_MOUSE || b->type != WYE_MSG_MOUSE || !wye_routine_is_move(a) || !wye_routine_is_move(b) ||
        !wye_mouse_unpack(a->payload, a->len, &ma) || !wye_mouse_unpack(b->payload, b->len, &mb) ||
        ma.buttons != mb.buttons) {
        return false;
    }
    int dx = ma.dx + mb.dx, dy = ma.dy + mb.dy;
    if (dx < INT16_MIN || dx > INT16_MAX || dy < INT16_MIN || dy > INT16_MAX) {
        return false;
    }
    ma.dx = (int16_t)dx;
    ma.dy = (int16_t)dy;
    a->len = (uint8_t)wye_mouse_pack(&ma, a->payload);
    return true;
}

static bool merge_abs(struct wye_routine_event *a, const struct wye_routine_event *b)
{
    struct wye_abs_mouse ma, mb;
    if (a->type != WYE_MSG_ABS_MOUSE || b->type != WYE_MSG_ABS_MOUSE || !wye_abs_mouse_unpack(a->payload, a->len, &ma) ||
        !wye_abs_mouse_unpack(b->payload, b->len, &mb) || ma.buttons != mb.buttons || ma.wheel || ma.pan || mb.wheel ||
        mb.pan) {
        return false;
    }
    a->len = b->len;
    memcpy(a->payload, b->payload, b->len); /* only the last position matters */
    return true;
}

/* True if this event changes nothing, given the state so far (which it then updates). */
static bool is_noop(const struct wye_routine_event *e, struct wye_routine_event *last_kbd, uint8_t *buttons)
{
    struct wye_mouse m;
    if (e->type == WYE_MSG_KEYBOARD) {
        bool same = last_kbd->len == e->len && memcmp(last_kbd->payload, e->payload, e->len) == 0;
        *last_kbd = *e;
        return same;
    }
    if (e->type == WYE_MSG_MOUSE && wye_mouse_unpack(e->payload, e->len, &m)) {
        bool same = !m.dx && !m.dy && !m.wheel && !m.pan && m.buttons == *buttons;
        *buttons = m.buttons;
        return same;
    }
    return false;
}

size_t wye_routine_doc_compress(struct wye_routine_doc *d, const struct wye_routine_compress *o)
{
    size_t kept = 0, before = d->count;
    struct wye_routine_event last_kbd = {.type = WYE_MSG_KEYBOARD, .len = WYE_KEYBOARD_LEN};
    uint8_t buttons = 0;
    uint32_t group_start = 0;
    for (size_t i = 0; i < d->count; ++i) {
        const struct wye_routine_event e = d->ev[i];
        if (is_noop(&e, &last_kbd, &buttons)) {
            continue;
        }
        struct wye_routine_event *prev = kept ? &d->ev[kept - 1] : NULL;
        if (o->merge_moves && prev && e.t - group_start < o->merge_ms && (merge_rel(prev, &e) || merge_abs(prev, &e))) {
            prev->t = e.t; /* the pointer gets there when it really did */
            continue;
        }
        group_start = e.t;
        d->ev[kept++] = e;
    }
    d->count = kept;
    if (o->cap_pauses && kept) {
        uint32_t cut = d->ev[0].t;
        d->ev[0].t = 0;
        for (size_t i = 1; i < kept; ++i) {
            uint32_t at = d->ev[i].t - cut, gap = at - d->ev[i - 1].t;
            if (gap > o->max_pause_ms) {
                cut += gap - o->max_pause_ms;
                at = d->ev[i - 1].t + o->max_pause_ms;
            }
            d->ev[i].t = at;
        }
    }
    return before - kept;
}
