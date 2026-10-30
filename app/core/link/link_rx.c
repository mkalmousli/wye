/* Handling packets from the bridge and target. */
#include "link.h"

#include "log.h"

static struct wye_node_info *node_of(struct wye_link *l, uint8_t node)
{
    if (node == WYE_NODE_BRIDGE) {
        return &l->info.bridge;
    }
    return node == WYE_NODE_TARGET ? &l->info.target : NULL;
}

static void on_hello_ack(struct wye_link *l, struct wye_node_info *n, const struct wye_packet *pkt,
                         uint64_t now)
{
    struct wye_hello_ack a;
    if (!wye_hello_ack_unpack(pkt->payload, pkt->len, &a)) {
        return;
    }
    bool was_present = n->present;
    n->hello = a;
    n->present = true;
    n->last_rx = now;
    if (!was_present) {
        LOG_I("link: %s firmware %u.%u.%u (protocol %u)", pkt->node == WYE_NODE_BRIDGE ? "bridge" : "target",
              a.fw_major, a.fw_minor, a.fw_patch, a.proto_version);
    }
    if (pkt->node == WYE_NODE_BRIDGE && l->info.state == WYE_LINK_HANDSHAKE) {
        l->next_bridge_ping = now + WYE_LINK_TIMING_BRIDGE_PING_MS;
        l->next_target_hello = now;
        wye_link_set_state(l, WYE_LINK_BRIDGE);
    } else if (pkt->node == WYE_NODE_TARGET && l->info.state == WYE_LINK_BRIDGE) {
        l->next_target_ping = now;
        wye_link_set_state(l, WYE_LINK_READY);
    }
}

static void on_pong(struct wye_link *l, struct wye_node_info *n, const struct wye_packet *pkt,
                    uint64_t now)
{
    struct wye_pong p;
    if (!wye_pong_unpack(pkt->payload, pkt->len, &p)) {
        return;
    }
    if (pkt->node == WYE_NODE_TARGET && p.uptime_ms < n->pong.uptime_ms) {
        /* Restarted faster than the timeout notices (e.g. after an update): greet it again,
           so its firmware version is fresh and everyone sees it leave and come back. */
        LOG_I("link: target restarted");
        n->present = false;
        n->pong = p;
        l->next_target_hello = now;
        wye_link_set_state(l, WYE_LINK_BRIDGE);
        return;
    }
    n->pong = p;
    if (p.token == n->ping_token) {
        n->rtt_ms = (uint32_t)(now - n->ping_sent);
    }
    if (pkt->node == WYE_NODE_TARGET) {
        l->info.target_state = p.status;
        l->info.leds = p.leds;
    }
}

void wye_link_handle(struct wye_link *l, const struct wye_packet *pkt, uint64_t now)
{
    struct wye_node_info *n = node_of(l, pkt->node);
    if (!n) {
        return;
    }
    if (pkt->type == WYE_MSG_HELLO_ACK) {
        on_hello_ack(l, n, pkt, now);
    } else if (!n->present) {
        return; /* ignore traffic until the node has identified itself */
    }
    n->last_rx = now;
    if (pkt->node == WYE_NODE_TARGET) {
        l->info.bridge.last_rx = now; /* target traffic proves the bridge is alive */
    }
    switch (pkt->type) {
    case WYE_MSG_PONG: on_pong(l, n, pkt, now); break;
    case WYE_MSG_LEDS: if (pkt->len) l->info.leds = pkt->payload[0]; break;
    case WYE_MSG_TARGET_STATE: if (pkt->len) l->info.target_state = pkt->payload[0]; break;
    case WYE_MSG_LOG: LOG_I("device: %.*s", pkt->len, (const char *)pkt->payload); break;
    default: break;
    }
    if (l->listener.packet) {
        l->listener.packet(l->listener.ctx, pkt);
    }
}
