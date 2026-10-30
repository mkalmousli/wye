/*
 * Target node logic: receives input state from the host over UART and turns
 * it into USB HID reports. Includes a failsafe that releases everything if
 * the host goes silent, so keys can never stay stuck on the target PC.
 */
#ifndef WYE_TARGET_H
#define WYE_TARGET_H

#include "node.h"
#include "target_disk.h"
#include "target_queue.h"

/* Host pings every 100 ms; after this much silence held input is released. */
#define WYE_TARGET_FAILSAFE_MS 600u
#define WYE_TARGET_WAKE_INTERVAL_MS 1000u

struct wye_target_hid {
    void *ctx;
    bool (*kbd_ready)(void *ctx);
    bool (*kbd_send)(void *ctx, const struct wye_keyboard *k);
    bool (*ptr_ready)(void *ctx);
    /* Relative deltas are pre-clamped to ±rel_max(ctx). */
    bool (*rel_send)(void *ctx, uint8_t buttons, int16_t dx, int16_t dy, int8_t wheel, int8_t pan);
    bool (*abs_send)(void *ctx, const struct wye_abs_mouse *m);
    int16_t (*rel_max)(void *ctx);  /* 127 in boot protocol, else 32767 */
    bool (*extra_ready)(void *ctx);
    bool (*consumer_send)(void *ctx, uint16_t usage);
    bool (*system_send)(void *ctx, uint8_t usage);
    void (*remote_wakeup)(void *ctx);
};

struct wye_target {
    struct wye_node node;
    const struct wye_target_hid *hid;
    struct wye_decoder decoder;
    struct wye_kbd_queue kbd;
    struct wye_ptr_queue ptr;
    struct wye_extra_queue extra;
    uint32_t last_rx_ms;
    uint32_t next_wake_ms;           /* remote-wakeup rate limit */
    uint8_t leds;
    uint8_t reported_leds;
    uint8_t reported_state;
    bool failsafe_tripped;
    struct wye_disk disk;            /* shared drive (target_disk*.c) */
};

void wye_target_init(struct wye_target *t, const struct wye_hello_ack *identity,
                     const struct wye_node_hooks *hooks, const struct wye_target_hid *hid);
/* Feed bytes received from the bridge (UART). */
void wye_target_rx(struct wye_target *t, const uint8_t *data, size_t len);
/* LED output report from the target PC. */
void wye_target_set_leds(struct wye_target *t, uint8_t leds);
/* Release all keys/buttons (also used on USB reset). */
void wye_target_release_all(struct wye_target *t);
/* Main loop work: emit reports, failsafe, notifications, deferred actions. */
void wye_target_task(struct wye_target *t);

static inline uint32_t wye_target_now(const struct wye_target *t)
{
    return t->node.hooks->now_ms(t->node.hooks->ctx);
}

/* Internal (target_input.c). */
void wye_target_dispatch_input(struct wye_target *t, const struct wye_packet *pkt);
void wye_target_flush_reports(struct wye_target *t);

#endif
