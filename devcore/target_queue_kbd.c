/* Keyboard and consumer/system report queues. */
#include "target_queue.h"

#include <string.h>

void wye_kbd_push(struct wye_kbd_queue *q, const struct wye_keyboard *k)
{
    if (memcmp(&q->last, k, sizeof(*k)) == 0) {
        return;
    }
    q->last = *k;
    if (q->count == WYE_KBD_QUEUE) {
        /* Full: overwrite the newest entry so the final state still wins. */
        q->items[(q->head + q->count - 1u) % WYE_KBD_QUEUE] = *k;
        return;
    }
    q->items[(q->head + q->count) % WYE_KBD_QUEUE] = *k;
    q->count++;
}

const struct wye_keyboard *wye_kbd_peek(const struct wye_kbd_queue *q)
{
    return q->count ? &q->items[q->head] : NULL;
}

void wye_kbd_pop(struct wye_kbd_queue *q)
{
    if (q->count) {
        q->head = (uint8_t)((q->head + 1u) % WYE_KBD_QUEUE);
        q->count--;
    }
}

bool wye_kbd_held(const struct wye_kbd_queue *q)
{
    static const uint8_t none[6] = {0};
    return q->last.modifiers != 0 || memcmp(q->last.keys, none, sizeof(none)) != 0;
}

void wye_extra_push(struct wye_extra_queue *q, bool system, uint16_t usage)
{
    if (system ? q->last_system == usage : q->last_consumer == usage) {
        return;
    }
    if (system) {
        q->last_system = (uint8_t)usage;
    } else {
        q->last_consumer = usage;
    }
    struct wye_extra_entry e = {system, usage};
    if (q->count == WYE_EXTRA_QUEUE) {
        q->items[(q->head + q->count - 1u) % WYE_EXTRA_QUEUE] = e;
        return;
    }
    q->items[(q->head + q->count) % WYE_EXTRA_QUEUE] = e;
    q->count++;
}

const struct wye_extra_entry *wye_extra_peek(const struct wye_extra_queue *q)
{
    return q->count ? &q->items[q->head] : NULL;
}

void wye_extra_pop(struct wye_extra_queue *q)
{
    if (q->count) {
        q->head = (uint8_t)((q->head + 1u) % WYE_EXTRA_QUEUE);
        q->count--;
    }
}

bool wye_extra_held(const struct wye_extra_queue *q)
{
    return q->last_consumer != 0 || q->last_system != 0;
}
