#include "input.h"

#include "hid_usage.h"
#include "wye_proto.h"

#include <string.h>

void wye_input_init(struct wye_input *in, const struct wye_input_out *out)
{
    memset(in, 0, sizeof(*in));
    in->out = *out;
    in->hotkey = HID_KEY_F12;
    in->sensitivity = 100;
    wye_input_default_chords(in->chord_keys);
}

static void send_usage(struct wye_input *in, const struct wye_input_event *ev)
{
    uint8_t p[2];
    if (ev->type == WYE_EV_CONSUMER) {
        wye_put_u16(p, ev->down ? ev->code : 0);
        in->out.send(in->out.ctx, WYE_MSG_CONSUMER, p, 2);
    } else {
        p[0] = ev->down ? (uint8_t)ev->code : 0;
        in->out.send(in->out.ctx, WYE_MSG_SYSTEM, p, 1);
    }
}

bool wye_input_event(struct wye_input *in, const struct wye_input_event *ev)
{
    switch (ev->type) {
    case WYE_EV_KEY:
        return wye_input_key(in, ev);
    case WYE_EV_CONSUMER:
    case WYE_EV_SYSTEM:
        if (in->mode == WYE_FWD_OFF) {
            return false;
        }
        send_usage(in, ev);
        return true;
    default:
        return wye_input_pointer(in, ev);
    }
}

/* Releases of keys held on the target must not leak to the local OS. */
static void remember_orphans(struct wye_input *in)
{
    for (uint8_t i = 0; i < in->remote.count; ++i) {
        wye_usage_set(&in->orphans, in->remote.keys[i], true);
    }
    for (uint8_t b = 0; b < 8; ++b) {
        if (in->remote.modifiers & (1u << b)) {
            wye_usage_set(&in->orphans, (uint8_t)(HID_KEY_LEFT_CTRL + b), true);
        }
    }
    in->orphan_buttons |= in->remote_buttons;
}

void wye_input_set_mode(struct wye_input *in, enum wye_fwd_mode mode)
{
    if (mode == in->mode) {
        return;
    }
    if (in->mode != WYE_FWD_OFF) {
        in->out.send(in->out.ctx, WYE_MSG_RELEASE_ALL, NULL, 0);
        remember_orphans(in);
    }
    wye_keystate_clear(&in->remote);
    in->remote_buttons = 0;
    in->rem_x = in->rem_y = in->wheel_acc = in->pan_acc = 0;
    in->mode = mode;
    if (in->out.mode_changed) {
        in->out.mode_changed(in->out.ctx, mode);
    }
}
