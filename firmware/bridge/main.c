/*
 * Bridge firmware: USB CDC from the controlling computer <-> UART to the
 * target Pico. Routing/protocol logic lives in devcore/bridge.c.
 */
#include "board.h"
#include "bridge.h"
#include "usb_common.h"
#include "usb_audio.h"

#include "pico/stdlib.h"

WYE_FW_ROLE_TAG("bridge");

static struct wye_bridge g_bridge;

static const struct wye_identity g_default_id = {
    0x1209, 0x0001, "Wye", "Wye Bridge", "WYE-BRIDGE",
};

static uint32_t now_ms(void *ctx) { (void)ctx; return board_now_ms(); }
static void reboot(void *ctx, uint8_t mode) { (void)ctx; board_reboot(mode); }
static void install(void *ctx, uint32_t size) { (void)ctx; fw_flash_install(size); }
static void uart_out(void *ctx, const uint8_t *d, size_t n) { (void)ctx; uart_link_write(d, n); }

void tud_umount_cb(void) { wye_usb_audio_reset(); }
void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; wye_usb_audio_suspend(); }

static void send_host(void *ctx, const uint8_t *d, size_t n)
{
    (void)ctx;
    /* A CDC reader may send HELLO before the host's DTR control transfer has
       completed. Protocol replies must not depend on modem-control state. */
    if (tud_mounted() && !tud_suspended() && tud_cdc_write_available() >= n) {
        tud_cdc_write(d, (uint32_t)n);
        tud_cdc_write_flush();
    }
}

static bool host_present(void *ctx)
{
    (void)ctx;
    return tud_mounted() && !tud_suspended() && tud_cdc_connected();
}

static uint8_t status(void *ctx) { return host_present(ctx) ? WYE_BSTATE_HOST_DTR : 0; }

static bool identity_get(void *ctx, struct wye_identity *id) { (void)ctx; *id = *identity_current(); return true; }
static bool identity_set(void *ctx, const struct wye_identity *id) { (void)ctx; return identity_save(id); }
static bool profile_get(void *ctx, struct wye_profile *f) { (void)ctx; *f = *profile_current(); return true; }
static bool profile_set(void *ctx, const struct wye_profile *f) { (void)ctx; return profile_save(f); }

/* Arduino-style recovery: opening the port at 1200 baud enters BOOTSEL. */
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *coding)
{
    (void)itf;
    if (coding->bit_rate == 1200) {
        board_reboot(WYE_REBOOT_BOOTSEL);
    }
}

int main(void)
{
    static struct wye_node_hooks hooks = {
        .now_ms = now_ms, .send_host = send_host, .status = status, .reboot = reboot,
        .install = install, .identity_get = identity_get, .identity_set = identity_set, .profile_get = profile_get, .profile_set = profile_set,
        .audio_rx = wye_usb_audio_rx,
    };
    static const struct wye_bridge_io io = {NULL, uart_out, host_present};
    struct wye_hello_ack hello;

    wye_crash_start();
    identity_init(&g_default_id);
    profile_init();
    hooks.flash = fw_flash_ops(wye_fw_role_tag);
    board_hello(&hello, WYE_ROLE_BRIDGE,
                WYE_CAP_FW_UPDATE | WYE_CAP_IDENTITY | WYE_CAP_AUDIO | WYE_CAP_PROFILE);
    wye_usb_audio_init(2, 0x03, 0x83, true); /* as in usb_desc.c */
    tusb_init();
    uart_link_init();
    wye_bridge_init(&g_bridge, &hello, &hooks, &io);

    uint8_t buf[256];
    for (;;) {
        tud_task();
        if (tud_cdc_available()) {
            uint32_t n = tud_cdc_read(buf, sizeof(buf));
            wye_bridge_host_rx(&g_bridge, buf, n);
        }
        size_t n = uart_link_read(buf, sizeof(buf));
        if (n) {
            wye_bridge_uart_rx(&g_bridge, buf, n);
        }
        wye_bridge_task(&g_bridge);
        wye_usb_audio_task();
        wye_crash_feed();
        wye_crash_tell(&g_bridge.node, host_present(NULL), board_now_ms());
        board_led(host_present(NULL));
    }
}
