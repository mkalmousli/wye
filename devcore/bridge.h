/*
 * Bridge node logic (the Pico plugged into the controlling computer).
 * Routes host frames: node BRIDGE is handled locally, node TARGET is
 * forwarded verbatim over UART. Target audio goes to local USB audio;
 * other target packets go to the host's CDC port.
 * If the host disappears (port closed, USB suspend/unplug) the target is
 * told to release all input immediately.
 */
#ifndef WYE_BRIDGE_H
#define WYE_BRIDGE_H

#include "node.h"

struct wye_bridge_io {
    void *ctx;
    void (*uart_write)(void *ctx, const uint8_t *data, size_t len);
    bool (*host_present)(void *ctx);  /* USB mounted, not suspended, DTR set */
};

struct wye_bridge {
    struct wye_node node;
    const struct wye_bridge_io *io;
    struct wye_decoder host_decoder;
    struct wye_decoder uart_decoder;
    bool host_was_present;
};

void wye_bridge_init(struct wye_bridge *b, const struct wye_hello_ack *identity,
                     const struct wye_node_hooks *hooks, const struct wye_bridge_io *io);
void wye_bridge_host_rx(struct wye_bridge *b, const uint8_t *data, size_t len);
void wye_bridge_uart_rx(struct wye_bridge *b, const uint8_t *data, size_t len);
void wye_bridge_task(struct wye_bridge *b);

#endif
