#include "node.h"

#include <string.h>

void wye_node_init(struct wye_node *n, uint8_t node_id, const struct wye_hello_ack *identity,
                   const struct wye_node_hooks *hooks)
{
    memset(n, 0, sizeof(*n));
    n->node_id = node_id;
    n->identity = *identity;
    n->hooks = hooks;
    wye_fwupdate_reset(&n->fw);
}

void wye_node_send(struct wye_node *n, uint8_t type, uint8_t seq, const void *payload, size_t len)
{
    uint8_t buf[WYE_MAX_ENCODED];
    size_t enc = wye_packet_encode(n->node_id, type, seq, payload, len, buf, sizeof(buf));
    if (enc) {
        n->hooks->send_host(n->hooks->ctx, buf, enc);
    }
}

void wye_node_error(struct wye_node *n, uint8_t seq, uint8_t code, uint8_t type)
{
    uint8_t p[2] = {code, type};
    wye_node_send(n, WYE_MSG_ERROR, seq, p, sizeof(p));
}

void wye_node_count_error(struct wye_node *n)
{
    if (n->rx_errors < 0xffffu) {
        n->rx_errors++;
    }
}

static void reply_pong(struct wye_node *n, const struct wye_packet *pkt)
{
    const struct wye_node_hooks *h = n->hooks;
    struct wye_pong pong = {
        .token = wye_get_u32(pkt->payload),
        .status = h->status ? h->status(h->ctx) : 0,
        .leds = h->leds ? h->leds(h->ctx) : 0,
        .rx_errors = n->rx_errors,
        .uptime_ms = h->now_ms(h->ctx),
    };
    uint8_t buf[WYE_PONG_LEN];
    wye_node_send(n, WYE_MSG_PONG, pkt->seq, buf, wye_pong_pack(&pong, buf));
}

bool wye_node_handle(struct wye_node *n, const struct wye_packet *pkt)
{
    switch (pkt->type) {
    case WYE_MSG_HELLO: {
        uint8_t buf[WYE_HELLO_ACK_LEN];
        if (pkt->len >= 1 && pkt->payload[0] != WYE_PROTO_VERSION) {
            wye_node_error(n, pkt->seq, WYE_ERR_BAD_VERSION, pkt->type);
        }
        /* Always answer, so the host can identify and update old devices. */
        wye_node_send(n, WYE_MSG_HELLO_ACK, pkt->seq, buf, wye_hello_ack_pack(&n->identity, buf));
        return true;
    }
    case WYE_MSG_PING:
        if (pkt->len < 4) {
            wye_node_error(n, pkt->seq, WYE_ERR_BAD_LENGTH, pkt->type);
        } else {
            reply_pong(n, pkt);
        }
        return true;
    default:
        return wye_node_handle_maint(n, pkt);
    }
}

void wye_node_task(struct wye_node *n)
{
    const struct wye_node_hooks *h = n->hooks;
    if (n->pending == WYE_PENDING_NONE || (int32_t)(h->now_ms(h->ctx) - n->pending_at) < 0) {
        return;
    }
    uint8_t kind = n->pending;
    n->pending = WYE_PENDING_NONE;
    if (kind == WYE_PENDING_INSTALL && h->install) {
        h->install(h->ctx, n->fw.size);
    } else if (h->reboot) {
        h->reboot(h->ctx, n->pending_mode);
    }
}
