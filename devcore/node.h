/*
 * Behaviour common to both firmware nodes (bridge and target): handshake,
 * heartbeat replies, reboot, firmware update and USB identity messages.
 * Hardware access goes through wye_node_hooks so this code runs unchanged
 * in firmware and in the desktop test simulator.
 */
#ifndef WYE_NODE_H
#define WYE_NODE_H

#include "fwupdate.h"
#include "wye_codec.h"
#include "wye_msg.h"

/* Delay between acknowledging and acting, so the reply drains first. */
#define WYE_NODE_ACT_DELAY_MS 60u

struct wye_node_hooks {
    void *ctx;
    uint32_t (*now_ms)(void *ctx);
    void (*send_host)(void *ctx, const uint8_t *data, size_t len);
    uint8_t (*status)(void *ctx);   /* WYE_TSTATE_* / WYE_BSTATE_* */
    uint8_t (*leds)(void *ctx);
    void (*reboot)(void *ctx, uint8_t mode);
    void (*install)(void *ctx, uint32_t size);  /* copy staged image, reboot */
    bool (*identity_get)(void *ctx, struct wye_identity *id);
    bool (*identity_set)(void *ctx, const struct wye_identity *id);
    bool (*functions_get)(void *ctx, struct wye_functions *f);    /* NULL on the bridge */
    bool (*functions_set)(void *ctx, const struct wye_functions *f);
    bool (*profile_get)(void *ctx, struct wye_profile *f);
    bool (*profile_set)(void *ctx, const struct wye_profile *f);
    const struct wye_flash_ops *flash;          /* NULL: updates unsupported */
    void (*audio_rx)(void *ctx, const uint8_t *data, size_t len);
};

enum wye_node_pending { WYE_PENDING_NONE = 0, WYE_PENDING_REBOOT, WYE_PENDING_INSTALL };

struct wye_node {
    uint8_t node_id;               /* WYE_NODE_BRIDGE / WYE_NODE_TARGET */
    struct wye_hello_ack identity;
    const struct wye_node_hooks *hooks;
    struct wye_fwupdate fw;
    uint16_t rx_errors;
    uint8_t pending;               /* enum wye_node_pending */
    uint8_t pending_mode;
    uint32_t pending_at;
};

void wye_node_init(struct wye_node *n, uint8_t node_id, const struct wye_hello_ack *identity,
                   const struct wye_node_hooks *hooks);
/* Handles session and maintenance messages; returns true if consumed. */
bool wye_node_handle(struct wye_node *n, const struct wye_packet *pkt);
void wye_node_send(struct wye_node *n, uint8_t type, uint8_t seq, const void *payload, size_t len);
void wye_node_error(struct wye_node *n, uint8_t seq, uint8_t code, uint8_t type);
void wye_node_count_error(struct wye_node *n);
/* Runs deferred reboot/install; call from the main loop. */
void wye_node_task(struct wye_node *n);

/* Internal: maintenance message handlers (node_maint.c). */
bool wye_node_handle_maint(struct wye_node *n, const struct wye_packet *pkt);
bool wye_node_handle_ident(struct wye_node *n, const struct wye_packet *pkt);
void wye_node_schedule(struct wye_node *n, uint8_t kind, uint8_t mode);

#endif
