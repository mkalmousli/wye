/*
 * Target firmware: UART from the bridge -> USB HID on the target PC.
 * Report queueing, failsafe and protocol logic live in devcore/target*.c.
 */
#include "board.h"
#include "hid_desc.h"
#include "hid_out.h"
#include "target_main.h"
#include "usb_common.h"
#include "usb_audio.h"

#include "pico/stdlib.h"

WYE_FW_ROLE_TAG("target");

static struct wye_target g_target;

static const struct wye_identity g_default_id = {
    0x1209, 0x0002, "Wye", "Wye Keyboard and Mouse", "WYE-TARGET",
};

static const struct wye_functions g_default_fn = {
    WYE_FN_DRIVE | WYE_FN_AUDIO, "Wye", "Shared Folder",
};

static uint32_t now_ms(void *ctx) { (void)ctx; return board_now_ms(); }
static void reboot(void *ctx, uint8_t mode) { (void)ctx; board_reboot(mode); }
static void install(void *ctx, uint32_t size) { (void)ctx; fw_flash_install(size); }
static void to_bridge(void *ctx, const uint8_t *d, size_t n) { (void)ctx; uart_link_write(d, n); }
static uint8_t leds(void *ctx) { (void)ctx; return g_target.leds; }

static uint8_t status(void *ctx)
{
    (void)ctx;
    uint8_t s = 0;
    s |= tud_mounted() ? WYE_TSTATE_USB_MOUNTED : 0;
    s |= tud_suspended() ? WYE_TSTATE_USB_SUSPENDED : 0;
    s |= tud_hid_n_get_protocol(HID_INST_KBD) == HID_PROTOCOL_BOOT ? WYE_TSTATE_KBD_BOOT : 0;
    return s;
}

static bool identity_get(void *ctx, struct wye_identity *id) { (void)ctx; *id = *identity_current(); return true; }
static bool identity_set(void *ctx, const struct wye_identity *id) { (void)ctx; return identity_save(id); }
static bool profile_get(void *ctx, struct wye_profile *f) { (void)ctx; *f = *profile_current(); return true; }
static bool profile_set(void *ctx, const struct wye_profile *f) { (void)ctx; return profile_save(f); }
static bool functions_get(void *ctx, struct wye_functions *f) { (void)ctx; *f = *functions_current(); return true; }
static bool functions_set(void *ctx, const struct wye_functions *f) { (void)ctx; return functions_save(f); }

struct wye_target *target_get(void) { return &g_target; }

void target_pump(void)
{
    uint8_t buf[256];
    size_t n = uart_link_read(buf, sizeof(buf));
    if (n) {
        wye_target_rx(&g_target, buf, n);
    }
    wye_target_task(&g_target);
    board_led(!g_target.failsafe_tripped);
    wye_crash_feed(); /* also runs inside the drive's wait loops */
    wye_crash_tell(&g_target.node, g_target.last_rx_ms != 0, board_now_ms());
}

/* Never leave keys down across a USB reset or sleep transition. */
void tud_umount_cb(void) { wye_target_release_all(&g_target); wye_usb_audio_reset(); }
void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; wye_usb_audio_suspend(); }

int main(void)
{
    static struct wye_node_hooks hooks = {
        .now_ms = now_ms, .send_host = to_bridge, .status = status, .leds = leds,
        .reboot = reboot, .install = install, .identity_get = identity_get,
        .identity_set = identity_set, .profile_get = profile_get, .profile_set = profile_set, .functions_get = functions_get, .functions_set = functions_set,
        .audio_rx = wye_usb_audio_rx,
    };
    struct wye_hello_ack hello;

    wye_crash_start();
    identity_init(&g_default_id);
    profile_init();
    functions_init(&g_default_fn);
    uint8_t flags = functions_current()->flags;
    wye_usb_audio_init(flags & WYE_FN_DRIVE ? 4 : 3, 0x05, 0x85, (flags & WYE_FN_AUDIO) != 0);
    hooks.flash = fw_flash_ops(wye_fw_role_tag);
    board_hello(&hello, WYE_ROLE_TARGET,
                WYE_CAP_FW_UPDATE | WYE_CAP_BOOT_KEYBOARD | WYE_CAP_MOUSE16 |
                    WYE_CAP_REMOTE_WAKEUP | WYE_CAP_ABS_MOUSE | WYE_CAP_IDENTITY | WYE_CAP_DISK | WYE_CAP_AUDIO | WYE_CAP_PROFILE);
    tusb_init();
    uart_link_init();
    wye_target_init(&g_target, &hello, &hooks, hid_out());

    for (;;) {
        tud_task();
        target_pump();
        wye_usb_audio_task();
    }
}
