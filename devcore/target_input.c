/* Target: turning input messages into queued HID reports, and emitting them. */
#include "target.h"

static int32_t clamp_take(int32_t *pending, int32_t limit)
{
    int32_t v = *pending > limit ? limit : (*pending < -limit ? -limit : *pending);
    *pending -= v;
    return v;
}

static void maybe_wake(struct wye_target *t, bool pressed)
{
    const struct wye_node_hooks *h = t->node.hooks;
    uint32_t now = h->now_ms(h->ctx);
    bool suspended = h->status && (h->status(h->ctx) & WYE_TSTATE_USB_SUSPENDED);
    if (pressed && suspended && t->hid->remote_wakeup && (int32_t)(now - t->next_wake_ms) >= 0) {
        t->next_wake_ms = now + WYE_TARGET_WAKE_INTERVAL_MS;
        t->hid->remote_wakeup(t->hid->ctx);
    }
}

void wye_target_dispatch_input(struct wye_target *t, const struct wye_packet *pkt)
{
    struct wye_keyboard k;
    struct wye_mouse m;
    struct wye_abs_mouse a;
    if (pkt->type == WYE_MSG_KEYBOARD && wye_keyboard_unpack(pkt->payload, pkt->len, &k)) {
        wye_kbd_push(&t->kbd, &k);
        maybe_wake(t, wye_kbd_held(&t->kbd));
    } else if (pkt->type == WYE_MSG_MOUSE && wye_mouse_unpack(pkt->payload, pkt->len, &m)) {
        wye_ptr_push_rel(&t->ptr, &m);
        maybe_wake(t, m.buttons != 0);
    } else if (pkt->type == WYE_MSG_ABS_MOUSE && wye_abs_mouse_unpack(pkt->payload, pkt->len, &a)) {
        wye_ptr_push_abs(&t->ptr, &a);
        maybe_wake(t, a.buttons != 0);
    } else if (pkt->type == WYE_MSG_CONSUMER && pkt->len >= 2) {
        wye_extra_push(&t->extra, false, wye_get_u16(pkt->payload));
    } else if (pkt->type == WYE_MSG_SYSTEM && pkt->len >= 1) {
        wye_extra_push(&t->extra, true, pkt->payload[0]);
    } else if (pkt->type == WYE_MSG_RELEASE_ALL) {
        wye_target_release_all(t);
    } else {
        wye_node_error(&t->node, pkt->seq, WYE_ERR_UNKNOWN_TYPE, pkt->type);
    }
}

static void flush_ptr(struct wye_target *t)
{
    const struct wye_target_hid *hid = t->hid;
    struct wye_ptr_entry *e = wye_ptr_peek(&t->ptr);
    if (!e || !hid->ptr_ready(hid->ctx)) {
        return;
    }
    bool ok;
    int8_t wheel = (int8_t)clamp_take(&e->wheel, 127);
    int8_t pan = (int8_t)clamp_take(&e->pan, 127);
    if (e->absolute) {
        struct wye_abs_mouse a = {e->buttons, e->x, e->y, wheel, pan};
        ok = hid->abs_send(hid->ctx, &a);
    } else {
        int32_t lim = hid->rel_max(hid->ctx);
        int16_t dx = (int16_t)clamp_take(&e->dx, lim);
        int16_t dy = (int16_t)clamp_take(&e->dy, lim);
        ok = hid->rel_send(hid->ctx, e->buttons, dx, dy, wheel, pan);
    }
    e->sent = e->sent || ok;
    if (e->sent && !e->dx && !e->dy && !e->wheel && !e->pan) {
        wye_ptr_pop(&t->ptr);
    }
}

void wye_target_flush_reports(struct wye_target *t)
{
    const struct wye_target_hid *hid = t->hid;
    const struct wye_keyboard *k = wye_kbd_peek(&t->kbd);
    if (k && hid->kbd_ready(hid->ctx) && hid->kbd_send(hid->ctx, k)) {
        wye_kbd_pop(&t->kbd);
    }
    flush_ptr(t);
    const struct wye_extra_entry *x = wye_extra_peek(&t->extra);
    if (x && hid->extra_ready(hid->ctx)) {
        bool ok = x->system ? hid->system_send(hid->ctx, (uint8_t)x->usage)
                            : hid->consumer_send(hid->ctx, x->usage);
        if (ok) {
            wye_extra_pop(&t->extra);
        }
    }
}
