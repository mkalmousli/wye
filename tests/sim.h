/*
 * Device simulator: runs the real devcore firmware logic (bridge + target)
 * against fake hardware, so link, failsafe and update behaviour can be
 * tested on the desktop.
 */
#ifndef WYE_SIM_H
#define WYE_SIM_H

#include "bridge.h"
#include "target.h"

#define SIM_FLASH_SIZE (256u * 1024u)
#define SIM_MAX_REPORTS 256
#define SIM_IO_MAX 65536

struct sim_report {
    char kind;                 /* 'k' keyboard, 'r' rel, 'a' abs, 'c' consumer, 's' system */
    struct wye_keyboard kbd;
    uint8_t buttons;
    int32_t dx, dy, x, y;
    uint16_t usage;
};

struct sim_flash {
    uint8_t mem[SIM_FLASH_SIZE];
    struct wye_flash_ops ops;
    uint32_t installed_size;
};

struct sim_device {
    uint32_t now;
    bool hid_ready;
    bool suspended;
    bool cable_connected;      /* UART between the two Picos */
    bool host_present;
    int16_t rel_max;
    int wakeups, reboots;
    struct sim_report reports[SIM_MAX_REPORTS];
    int report_count;
    struct wye_target target;
    struct wye_bridge bridge;
    struct wye_node_hooks target_hooks, bridge_hooks;
    struct wye_target_hid hid;
    struct wye_bridge_io bridge_io;
    struct sim_flash target_flash, bridge_flash;
    struct wye_functions functions;    /* what the target stores */
    uint8_t to_host[SIM_IO_MAX];  /* bytes the bridge sent to the host */
    size_t to_host_len;
};

void sim_init(struct sim_device *s);
void sim_advance(struct sim_device *s, uint32_t ms);  /* runs tasks every 1 ms */
void sim_host_write(struct sim_device *s, const uint8_t *data, size_t len);
size_t sim_host_read(struct sim_device *s, uint8_t *out, size_t cap);
/* Encode and send one packet from the host. */
void sim_send(struct sim_device *s, uint8_t node, uint8_t type, const void *payload, size_t len);

/* Internal (sim_hw.c). */
void sim_setup_hid(struct sim_device *s);
void sim_setup_flash(struct sim_flash *f);
bool sim_fn_get(void *ctx, struct wye_functions *f);
bool sim_fn_set(void *ctx, const struct wye_functions *f);

#endif
