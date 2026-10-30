#include "target.h"

#include <string.h>

void wye_target_init(struct wye_target *t, const struct wye_hello_ack *identity,
                     const struct wye_node_hooks *hooks, const struct wye_target_hid *hid)
{
    memset(t, 0, sizeof(*t));
    wye_node_init(&t->node, WYE_NODE_TARGET, identity, hooks);
    t->hid = hid;
    wye_decoder_init(&t->decoder);
    t->last_rx_ms = hooks->now_ms(hooks->ctx);
}

static void handle_packet(struct wye_target *t, const struct wye_packet *pkt)
{
    if (pkt->node != WYE_NODE_TARGET) {
        return; /* not for us (e.g. echo of bridge traffic) */
    }
    if (pkt->type == WYE_MSG_AUDIO) {
        if (pkt->len && pkt->len <= WYE_AUDIO_CHUNK && !(pkt->len % WYE_AUDIO_SAMPLE_BYTES) &&
            t->node.hooks->audio_rx) {
            t->node.hooks->audio_rx(t->node.hooks->ctx, pkt->payload, pkt->len);
        }
        return; /* Audio must never keep held keyboard/mouse input alive. */
    }
    t->last_rx_ms = t->node.hooks->now_ms(t->node.hooks->ctx);
    t->failsafe_tripped = false;
    if (!wye_node_handle(&t->node, pkt) && !wye_disk_handle(t, pkt)) {
        wye_target_dispatch_input(t, pkt);
    }
}

void wye_target_rx(struct wye_target *t, const uint8_t *data, size_t len)
{
    struct wye_packet pkt;
    for (size_t i = 0; i < len; ++i) {
        enum wye_decode r = wye_decoder_feed(&t->decoder, data[i], &pkt);
        if (r == WYE_DECODE_PACKET) {
            handle_packet(t, &pkt);
        } else if (r == WYE_DECODE_ERROR) {
            wye_node_count_error(&t->node);
        }
    }
}

void wye_target_set_leds(struct wye_target *t, uint8_t leds)
{
    t->leds = leds;
}

void wye_target_release_all(struct wye_target *t)
{
    static const struct wye_keyboard no_keys;
    static const struct wye_mouse no_mouse;
    wye_kbd_push(&t->kbd, &no_keys);
    wye_ptr_push_rel(&t->ptr, &no_mouse);
    wye_extra_push(&t->extra, false, 0);
    wye_extra_push(&t->extra, true, 0);
}

static void notify_changes(struct wye_target *t)
{
    const struct wye_node_hooks *h = t->node.hooks;
    uint8_t state = h->status ? h->status(h->ctx) : 0;
    if (state != t->reported_state) {
        t->reported_state = state;
        wye_node_send(&t->node, WYE_MSG_TARGET_STATE, 0, &state, 1);
    }
    if (t->leds != t->reported_leds) {
        t->reported_leds = t->leds;
        wye_node_send(&t->node, WYE_MSG_LEDS, 0, &t->leds, 1);
    }
}

static bool anything_held(const struct wye_target *t)
{
    return wye_kbd_held(&t->kbd) || t->ptr.last_buttons != 0 || wye_extra_held(&t->extra);
}

void wye_target_task(struct wye_target *t)
{
    const struct wye_node_hooks *h = t->node.hooks;
    uint32_t now = h->now_ms(h->ctx);
    if (!t->failsafe_tripped && now - t->last_rx_ms > WYE_TARGET_FAILSAFE_MS) {
        t->failsafe_tripped = true;
        if (anything_held(t)) {
            wye_target_release_all(t);
        }
    }
    wye_target_flush_reports(t);
    notify_changes(t);
    wye_disk_task(t);
    wye_node_task(&t->node);
}
