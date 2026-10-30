#include "sim.h"

#include <string.h>

static uint32_t now_ms(void *ctx) { return ((struct sim_device *)ctx)->now; }

static uint8_t target_status(void *ctx)
{
    struct sim_device *s = ctx;
    return (uint8_t)(WYE_TSTATE_USB_MOUNTED | (s->suspended ? WYE_TSTATE_USB_SUSPENDED : 0));
}

static uint8_t target_leds(void *ctx) { return ((struct sim_device *)ctx)->target.leds; }
static void reboot(void *ctx, uint8_t mode) { (void)mode; ((struct sim_device *)ctx)->reboots++; }

static void install_target(void *ctx, uint32_t size)
{
    ((struct sim_device *)ctx)->target_flash.installed_size = size;
}

/* Target -> bridge over UART. */
static void target_send(void *ctx, const uint8_t *d, size_t n)
{
    struct sim_device *s = ctx;
    if (s->cable_connected) {
        wye_bridge_uart_rx(&s->bridge, d, n);
    }
}

/* Bridge -> target over UART. */
static void bridge_uart(void *ctx, const uint8_t *d, size_t n)
{
    struct sim_device *s = ctx;
    if (s->cable_connected) {
        wye_target_rx(&s->target, d, n);
    }
}

/* Bridge -> host over USB. */
static void bridge_send(void *ctx, const uint8_t *d, size_t n)
{
    struct sim_device *s = ctx;
    if (s->host_present && s->to_host_len + n <= SIM_IO_MAX) {
        memcpy(s->to_host + s->to_host_len, d, n);
        s->to_host_len += n;
    }
}

static bool host_present(void *ctx) { return ((struct sim_device *)ctx)->host_present; }

void sim_init(struct sim_device *s)
{
    memset(s, 0, sizeof(*s));
    s->hid_ready = s->cable_connected = s->host_present = true;
    s->rel_max = 32767;
    sim_setup_hid(s);
    sim_setup_flash(&s->target_flash);
    sim_setup_flash(&s->bridge_flash);
    s->target_hooks = (struct wye_node_hooks){
        .ctx = s, .now_ms = now_ms, .send_host = target_send, .status = target_status,
        .leds = target_leds, .reboot = reboot, .install = install_target,
        .flash = &s->target_flash.ops, .functions_get = sim_fn_get, .functions_set = sim_fn_set,
    };
    s->bridge_hooks = (struct wye_node_hooks){
        .ctx = s, .now_ms = now_ms, .send_host = bridge_send, .reboot = reboot,
        .flash = &s->bridge_flash.ops,
    };
    s->bridge_io = (struct wye_bridge_io){s, bridge_uart, host_present};
    struct wye_hello_ack id = {WYE_PROTO_VERSION, WYE_ROLE_TARGET, 3, 0, 0, 1, 0x3f, {0}, 0};
    wye_target_init(&s->target, &id, &s->target_hooks, &s->hid);
    id.role = WYE_ROLE_BRIDGE;
    wye_bridge_init(&s->bridge, &id, &s->bridge_hooks, &s->bridge_io);
}

void sim_advance(struct sim_device *s, uint32_t ms)
{
    for (uint32_t i = 0; i < ms; ++i) {
        s->now++;
        wye_bridge_task(&s->bridge);
        wye_target_task(&s->target);
    }
}

void sim_host_write(struct sim_device *s, const uint8_t *data, size_t len)
{
    if (s->host_present) {
        wye_bridge_host_rx(&s->bridge, data, len);
    }
}

size_t sim_host_read(struct sim_device *s, uint8_t *out, size_t cap)
{
    size_t n = s->to_host_len < cap ? s->to_host_len : cap;
    memcpy(out, s->to_host, n);
    memmove(s->to_host, s->to_host + n, s->to_host_len - n);
    s->to_host_len -= n;
    return n;
}

void sim_send(struct sim_device *s, uint8_t node, uint8_t type, const void *payload, size_t len)
{
    uint8_t buf[WYE_MAX_ENCODED];
    size_t n = wye_packet_encode(node, type, 0, payload, len, buf, sizeof(buf));
    sim_host_write(s, buf, n);
}
