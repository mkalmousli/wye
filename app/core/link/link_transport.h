/*
 * Transport used by the link. Production code wires this to plat_serial;
 * tests wire it to the device simulator.
 */
#ifndef WYE_LINK_TRANSPORT_H
#define WYE_LINK_TRANSPORT_H

#include "plat_serial.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WYE_LINK_TIMING_SCAN_MS 500u
#define WYE_LINK_TIMING_HELLO_MS 200u
#define WYE_LINK_TIMING_HANDSHAKE_MS 1200u
#define WYE_LINK_TIMING_BLACKLIST_MS 4000u
#define WYE_LINK_TIMING_BRIDGE_PING_MS 500u
#define WYE_LINK_TIMING_BRIDGE_TIMEOUT_MS 2000u
#define WYE_LINK_TIMING_TARGET_HELLO_MS 300u
#define WYE_LINK_TIMING_TARGET_PING_MS 100u
#define WYE_LINK_TIMING_TARGET_TIMEOUT_MS 450u

enum wye_port_kind {
    WYE_PORT_IGNORE = 0,
    WYE_PORT_NATIVE,   /* known Wye bridge identity */
    WYE_PORT_LEGACY,    /* generic Pico serial: may be old firmware */
};

struct wye_link_transport {
    void *ctx;
    uint64_t (*now)(void *ctx);
    int (*list)(void *ctx, struct plat_port_info *out, int max);
    enum wye_port_kind (*classify)(void *ctx, const struct plat_port_info *port);
    void *(*open)(void *ctx, const char *path, char *err, size_t errlen);
    int (*write)(void *ctx, void *port, const uint8_t *data, size_t len); /* -1: gone */
    void (*close)(void *ctx, void *port);
    bool (*open_pending)(void *ctx); /* optional: NULL from open means still opening */
};

#endif
