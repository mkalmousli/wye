/*
 * Report queues for the target. HID endpoints are polled by the target PC
 * (typically 1 ms); these queues guarantee that every state transition
 * (e.g. a fast press + release) reaches the PC, while pure motion is
 * coalesced into the newest pending report.
 */
#ifndef WYE_TARGET_QUEUE_H
#define WYE_TARGET_QUEUE_H

#include "wye_msg.h"

#define WYE_KBD_QUEUE 16u
#define WYE_PTR_QUEUE 16u
#define WYE_EXTRA_QUEUE 8u

struct wye_kbd_queue {
    struct wye_keyboard items[WYE_KBD_QUEUE];
    uint8_t head, count;
    struct wye_keyboard last;      /* newest state, queued or sent */
};

struct wye_ptr_entry {
    uint8_t buttons;
    bool absolute;
    bool sent;                     /* emitted at least once */
    int32_t dx, dy;                /* relative: pending motion */
    uint16_t x, y;                 /* absolute: position */
    int32_t wheel, pan;
};

struct wye_ptr_queue {
    struct wye_ptr_entry items[WYE_PTR_QUEUE];
    uint8_t head, count;
    uint8_t last_buttons;
};

struct wye_extra_entry {
    bool system;                   /* false: consumer usage */
    uint16_t usage;
};

struct wye_extra_queue {
    struct wye_extra_entry items[WYE_EXTRA_QUEUE];
    uint8_t head, count;
    uint16_t last_consumer;
    uint8_t last_system;
};

void wye_kbd_push(struct wye_kbd_queue *q, const struct wye_keyboard *k);
const struct wye_keyboard *wye_kbd_peek(const struct wye_kbd_queue *q);
void wye_kbd_pop(struct wye_kbd_queue *q);
bool wye_kbd_held(const struct wye_kbd_queue *q);

void wye_ptr_push_rel(struct wye_ptr_queue *q, const struct wye_mouse *m);
void wye_ptr_push_abs(struct wye_ptr_queue *q, const struct wye_abs_mouse *m);
struct wye_ptr_entry *wye_ptr_peek(struct wye_ptr_queue *q);
void wye_ptr_pop(struct wye_ptr_queue *q);

void wye_extra_push(struct wye_extra_queue *q, bool system, uint16_t usage);
const struct wye_extra_entry *wye_extra_peek(const struct wye_extra_queue *q);
void wye_extra_pop(struct wye_extra_queue *q);
bool wye_extra_held(const struct wye_extra_queue *q);

#endif
