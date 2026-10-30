/* Pointer queue: relative and absolute reports with motion coalescing. */
#include "target_queue.h"

#include <string.h>

static struct wye_ptr_entry *tail(struct wye_ptr_queue *q)
{
    return q->count ? &q->items[(q->head + q->count - 1u) % WYE_PTR_QUEUE] : NULL;
}

/* Returns the entry new data should merge into, appending one if needed. */
static struct wye_ptr_entry *slot_for(struct wye_ptr_queue *q, uint8_t buttons, bool absolute)
{
    struct wye_ptr_entry *t = tail(q);
    bool mergeable = t && t->buttons == buttons && t->absolute == absolute && !t->sent;
    if (mergeable || (t && q->count == WYE_PTR_QUEUE)) {
        t->buttons = buttons; /* full: merge so the latest button state wins */
        t->absolute = absolute;
        return t;
    }
    t = &q->items[(q->head + q->count) % WYE_PTR_QUEUE];
    memset(t, 0, sizeof(*t));
    t->buttons = buttons;
    t->absolute = absolute;
    q->count++;
    return t;
}

void wye_ptr_push_rel(struct wye_ptr_queue *q, const struct wye_mouse *m)
{
    if (!m->dx && !m->dy && !m->wheel && !m->pan && m->buttons == q->last_buttons) {
        return;
    }
    struct wye_ptr_entry *e = slot_for(q, m->buttons, false);
    e->dx += m->dx;
    e->dy += m->dy;
    e->wheel += m->wheel;
    e->pan += m->pan;
    q->last_buttons = m->buttons;
}

void wye_ptr_push_abs(struct wye_ptr_queue *q, const struct wye_abs_mouse *m)
{
    struct wye_ptr_entry *e = slot_for(q, m->buttons, true);
    e->x = m->x;
    e->y = m->y;
    e->wheel += m->wheel;
    e->pan += m->pan;
    q->last_buttons = m->buttons;
}

struct wye_ptr_entry *wye_ptr_peek(struct wye_ptr_queue *q)
{
    return q->count ? &q->items[q->head] : NULL;
}

void wye_ptr_pop(struct wye_ptr_queue *q)
{
    if (q->count) {
        q->head = (uint8_t)((q->head + 1u) % WYE_PTR_QUEUE);
        q->count--;
    }
}
