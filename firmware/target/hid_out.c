/* Implements wye_target_hid on top of TinyUSB. */
#include "hid_out.h"
#include "hid_desc.h"

#include "tusb.h"

static bool kbd_ready(void *ctx) { (void)ctx; return tud_hid_n_ready(HID_INST_KBD); }
static bool ptr_ready(void *ctx) { (void)ctx; return tud_hid_n_ready(HID_INST_MOUSE) && tud_hid_n_ready(HID_INST_EXTRA); }
static bool extra_ready(void *ctx) { (void)ctx; return tud_hid_n_ready(HID_INST_EXTRA); }

static bool mouse_boot(void)
{
    return tud_hid_n_get_protocol(HID_INST_MOUSE) == HID_PROTOCOL_BOOT;
}

static int16_t rel_max(void *ctx)
{
    (void)ctx;
    return mouse_boot() ? 127 : 32767;
}

static bool kbd_send(void *ctx, const struct wye_keyboard *k)
{
    (void)ctx;
    return tud_hid_n_keyboard_report(HID_INST_KBD, 0, k->modifiers, k->keys);
}

static bool rel_send(void *ctx, uint8_t buttons, int16_t dx, int16_t dy, int8_t wheel, int8_t pan)
{
    (void)ctx;
    if (mouse_boot()) {
        return tud_hid_n_mouse_report(HID_INST_MOUSE, 0, buttons, (int8_t)dx, (int8_t)dy, wheel, pan);
    }
    struct mouse16_report r = {buttons, dx, dy, wheel, pan};
    return tud_hid_n_report(HID_INST_MOUSE, 0, &r, sizeof(r));
}

static bool abs_send(void *ctx, const struct wye_abs_mouse *m)
{
    (void)ctx;
    return tud_hid_n_abs_mouse_report(HID_INST_EXTRA, RID_ABS, m->buttons, (int16_t)m->x,
                                      (int16_t)m->y, m->wheel, m->pan);
}

static bool consumer_send(void *ctx, uint16_t usage)
{
    (void)ctx;
    return tud_hid_n_report(HID_INST_EXTRA, RID_CONSUMER, &usage, sizeof(usage));
}

static bool system_send(void *ctx, uint8_t usage)
{
    (void)ctx;
    return tud_hid_n_report(HID_INST_EXTRA, RID_SYSTEM, &usage, sizeof(usage));
}

static void remote_wakeup(void *ctx)
{
    (void)ctx;
    tud_remote_wakeup();
}

const struct wye_target_hid *hid_out(void)
{
    static const struct wye_target_hid hid = {
        .kbd_ready = kbd_ready, .kbd_send = kbd_send, .ptr_ready = ptr_ready,
        .rel_send = rel_send, .abs_send = abs_send, .rel_max = rel_max,
        .extra_ready = extra_ready, .consumer_send = consumer_send,
        .system_send = system_send, .remote_wakeup = remote_wakeup,
    };
    return &hid;
}
