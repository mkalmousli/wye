/* Handshake retries, heartbeats and timeouts. */
#include "link.h"

#include "util/util.h"

#define LEGACY_RETRY_MS 60000u

static void send_ping(struct wye_link *l, struct wye_node_info *n, uint8_t node, uint64_t now)
{
    uint8_t p[4];
    n->ping_token = ++l->ping_token;
    n->ping_sent = now;
    wye_put_u32(p, n->ping_token);
    wye_link_send(l, node, WYE_MSG_PING, p, sizeof(p));
}

static void send_hello(struct wye_link *l, uint8_t node)
{
    uint8_t v = WYE_PROTO_VERSION;
    wye_link_send(l, node, WYE_MSG_HELLO, &v, 1);
}

static void handshake_timers(struct wye_link *l, uint64_t now)
{
    if (now - l->state_since > WYE_LINK_TIMING_HANDSHAKE_MS) {
        bool legacy = l->legacy_candidate;
        char path[PLAT_PORT_PATH_MAX];
        wye_strcpy(path, sizeof(path), l->info.port);
        wye_link_blacklist(l, path, now + (legacy ? LEGACY_RETRY_MS : WYE_LINK_TIMING_BLACKLIST_MS));
        if (legacy) {
            wye_strcpy(l->info.legacy_port, sizeof(l->info.legacy_port), path);
            wye_link_set_state(l, WYE_LINK_LEGACY);
        }
        wye_link_close(l, "no handshake reply");
        return;
    }
    if (now >= l->next_bridge_hello) {
        l->next_bridge_hello = now + WYE_LINK_TIMING_HELLO_MS;
        send_hello(l, WYE_NODE_BRIDGE);
    }
}

static void target_timers(struct wye_link *l, uint64_t now)
{
    struct wye_node_info *t = &l->info.target;
    if (t->present && now - t->last_rx > WYE_LINK_TIMING_TARGET_TIMEOUT_MS) {
        t->present = false;
        wye_link_set_state(l, WYE_LINK_BRIDGE);
    }
    if (!t->present && now >= l->next_target_hello) {
        l->next_target_hello = now + WYE_LINK_TIMING_TARGET_HELLO_MS;
        send_hello(l, WYE_NODE_TARGET);
    }
    if (t->present && now >= l->next_target_ping) {
        l->next_target_ping = now + WYE_LINK_TIMING_TARGET_PING_MS;
        send_ping(l, t, WYE_NODE_TARGET, now);
    }
}

void wye_link_timers(struct wye_link *l, uint64_t now)
{
    if (l->info.state == WYE_LINK_HANDSHAKE) {
        handshake_timers(l, now);
        return;
    }
    if (now - l->info.bridge.last_rx > WYE_LINK_TIMING_BRIDGE_TIMEOUT_MS) {
        wye_link_blacklist(l, l->info.port, now + WYE_LINK_TIMING_SCAN_MS);
        wye_link_close(l, "bridge stopped responding");
        return;
    }
    if (now >= l->next_bridge_ping) {
        l->next_bridge_ping = now + WYE_LINK_TIMING_BRIDGE_PING_MS;
        send_ping(l, &l->info.bridge, WYE_NODE_BRIDGE, now);
    }
    if (l->port) {
        target_timers(l, now);
    }
}
