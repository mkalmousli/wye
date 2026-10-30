/* Mouse routing: buttons, motion scaling, wheel accumulation. */
#include "input.h"

#include "wye_proto.h"

#define WHEEL_UNIT 120

static int16_t clamp16(int v)
{
    return (int16_t)(v > 32767 ? 32767 : (v < -32767 ? -32767 : v));
}

static int8_t clamp8(int v)
{
    return (int8_t)(v > 127 ? 127 : (v < -127 ? -127 : v));
}

void wye_input_send_pointer(struct wye_input *in, int dx, int dy, int wheel, int pan)
{
    uint8_t buf[WYE_MOUSE_LEN];
    struct wye_mouse m = {in->remote_buttons, clamp16(dx), clamp16(dy), clamp8(wheel), clamp8(pan)};
    in->out.send(in->out.ctx, WYE_MSG_MOUSE, buf, wye_mouse_pack(&m, buf));
}

static int scale(int delta, int sensitivity, int *rem)
{
    int total = delta * sensitivity + *rem;
    int out = total / 100;
    *rem = total - out * 100;
    return out;
}

static bool button(struct wye_input *in, const struct wye_input_event *ev)
{
    uint8_t bit = (uint8_t)ev->code;
    if (!ev->down && (in->orphan_buttons & bit)) {
        in->orphan_buttons &= (uint8_t)~bit;
        if (in->mode == WYE_FWD_OFF) {
            return true;
        }
    }
    if (!ev->down && (in->local_buttons & bit)) {
        in->local_buttons &= (uint8_t)~bit;
        return false;
    }
    if (in->mode == WYE_FWD_OFF) {
        if (ev->down) {
            in->local_buttons |= bit;
        }
        return false;
    }
    uint8_t before = in->remote_buttons;
    in->remote_buttons = ev->down ? (uint8_t)(before | bit) : (uint8_t)(before & ~bit);
    if (in->remote_buttons != before) {
        wye_input_send_pointer(in, 0, 0, 0, 0);
    }
    return true;
}

static bool wheel(struct wye_input *in, const struct wye_input_event *ev)
{
    if (in->mode == WYE_FWD_OFF) {
        return false;
    }
    in->wheel_acc += in->invert_wheel ? -ev->dy : ev->dy;
    in->pan_acc += ev->dx;
    int w = in->wheel_acc / WHEEL_UNIT;
    int p = in->pan_acc / WHEEL_UNIT;
    in->wheel_acc -= w * WHEEL_UNIT;
    in->pan_acc -= p * WHEEL_UNIT;
    if (w || p) {
        wye_input_send_pointer(in, 0, 0, w, p);
    }
    return true;
}

bool wye_input_pointer(struct wye_input *in, const struct wye_input_event *ev)
{
    if (ev->type == WYE_EV_BUTTON) {
        return button(in, ev);
    }
    if (ev->type == WYE_EV_WHEEL) {
        return wheel(in, ev);
    }
    if (in->mode != WYE_FWD_RELATIVE) {
        return false; /* not forwarding: the pointer is this computer's */
    }
    int dx = scale(ev->dx, in->sensitivity, &in->rem_x);
    int dy = scale(ev->dy, in->sensitivity, &in->rem_y);
    if (dx || dy) {
        wye_input_send_pointer(in, dx, dy, 0, 0);
    }
    return true;
}
