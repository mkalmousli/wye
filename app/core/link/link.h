/*
 * Host side of the Wye link: discovers the bridge, handshakes with both
 * nodes, keeps heartbeats running and reports the connection state.
 *
 *   SEARCHING -> HANDSHAKE -> BRIDGE (target silent) <-> READY
 *        ^           |  no answer: LEGACY (old firmware, offer update)
 *        +-----------+-- port error / bridge timeout
 *
 * Single-threaded: the engine serialises access with its own lock.
 */
#ifndef WYE_LINK_H
#define WYE_LINK_H

#include "wye_codec.h"
#include "wye_msg.h"
#include "link_transport.h"

enum wye_link_state {
    WYE_LINK_SEARCHING = 0,
    WYE_LINK_HANDSHAKE,
    WYE_LINK_LEGACY,
    WYE_LINK_BRIDGE,
    WYE_LINK_READY,
};

struct wye_node_info {
    bool present;
    struct wye_hello_ack hello;
    struct wye_pong pong;
    uint32_t rtt_ms;
    uint64_t last_rx;
    uint32_t ping_token;   /* last ping sent to this node */
    uint64_t ping_sent;
};

struct wye_link_info {
    enum wye_link_state state;
    char port[PLAT_PORT_PATH_MAX];
    char legacy_port[PLAT_PORT_PATH_MAX];  /* last device that never answered */
    char product[PLAT_PORT_STR_MAX];       /* the bridge's USB product name (its sound card is called that) */
    struct wye_node_info bridge, target;
    uint8_t target_state;                  /* WYE_TSTATE_* */
    uint8_t leds;
    uint32_t rx_errors;
};

struct wye_link_listener {
    void *ctx;
    void (*packet)(void *ctx, const struct wye_packet *pkt);
    void (*state)(void *ctx, enum wye_link_state old_state, enum wye_link_state new_state);
};

#define WYE_LINK_BLACKLIST 8

struct wye_link {
    const struct wye_link_transport *tr;
    struct wye_link_listener listener;
    void *port;
    struct wye_link_info info;
    struct wye_decoder decoder;
    uint8_t seq;
    bool legacy_candidate;
    bool write_failed; /* closed by the I/O thread after any outstanding read */
    uint64_t state_since, next_scan, next_bridge_hello, next_bridge_ping;
    uint64_t next_target_hello, next_target_ping;
    uint32_t ping_token;
    struct { char path[PLAT_PORT_PATH_MAX]; uint64_t until; } blacklist[WYE_LINK_BLACKLIST];
};

void wye_link_init(struct wye_link *l, const struct wye_link_transport *tr,
                   const struct wye_link_listener *listener);
/* Feed received bytes (may be 0) and run timers. */
void wye_link_process(struct wye_link *l, const uint8_t *data, size_t len);
/* Encodes and writes one packet; false if no port or write failed. */
bool wye_link_send(struct wye_link *l, uint8_t node, uint8_t type, const void *payload, size_t len);
void wye_link_close(struct wye_link *l, const char *reason);
bool wye_link_target_ready(const struct wye_link *l);

/* Internal, split across link_*.c */
void wye_link_set_state(struct wye_link *l, enum wye_link_state s);
void wye_link_scan(struct wye_link *l, uint64_t now);
void wye_link_timers(struct wye_link *l, uint64_t now);
void wye_link_handle(struct wye_link *l, const struct wye_packet *pkt, uint64_t now);
void wye_link_blacklist(struct wye_link *l, const char *path, uint64_t until);

#endif
