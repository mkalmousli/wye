/* Translating evdev events, asking the sink, re-injecting what stays local. */
#include "evdev.h"

#include "wye_proto.h"

static uint16_t button_bit(uint16_t code)
{
    switch (code) {
    case BTN_LEFT: return WYE_BTN_LEFT;
    case BTN_RIGHT: return WYE_BTN_RIGHT;
    case BTN_MIDDLE: return WYE_BTN_MIDDLE;
    case BTN_SIDE: case BTN_BACK: return WYE_BTN_BACK;
    case BTN_EXTRA: case BTN_FORWARD: return WYE_BTN_FORWARD;
    default: return 0;
    }
}

/* Builds the neutral event; returns false if it has no meaning for the target. */
static bool translate(const struct input_event *ev, struct wye_input_event *out)
{
    *out = (struct wye_input_event){0};
    if (ev->type == EV_KEY) {
        out->down = ev->value != 0;
        out->repeat = ev->value == 2;
        if ((out->code = button_bit(ev->code))) {
            out->type = WYE_EV_BUTTON;
        } else if ((out->code = ev_key_to_hid(ev->code))) {
            out->type = WYE_EV_KEY;
        } else if ((out->code = ev_key_to_consumer(ev->code))) {
            out->type = WYE_EV_CONSUMER;
        } else if ((out->code = ev_key_to_system(ev->code))) {
            out->type = WYE_EV_SYSTEM;
        }
        return out->type != 0 && !(out->repeat && out->type != WYE_EV_KEY);
    }
    if (ev->type == EV_REL) {
        out->type = ev->code == REL_X || ev->code == REL_Y ? WYE_EV_MOVE : WYE_EV_WHEEL;
        out->dx = ev->code == REL_X ? ev->value : ev->code == REL_HWHEEL ? ev->value * 120 : 0;
        out->dy = ev->code == REL_Y ? ev->value : ev->code == REL_WHEEL ? ev->value * 120 : 0;
        return out->dx || out->dy;
    }
    return false;
}

static void reinject(struct ev_capture *c, const struct ev_device *d, const struct input_event *ev)
{
    bool pointer_event = ev->type == EV_REL || ev->type == EV_ABS ||
                         (ev->type == EV_KEY && ev->code >= BTN_MISC && ev->code < KEY_OK);
    if (ev->type == EV_SYN) {
        ui_emit(c->ui_kbd, EV_SYN, SYN_REPORT, 0);
        ui_emit(c->ui_ptr, EV_SYN, SYN_REPORT, 0);
    } else if (pointer_event) {
        ui_emit(c->ui_ptr, ev->type, ev->code, ev->value);
    } else if (ev->type == EV_KEY || (ev->type == EV_MSC && d->keyboard)) {
        ui_emit(c->ui_kbd, ev->type, ev->code, ev->value);
    }
}

static void dispatch(struct ev_capture *c, const struct ev_device *d, const struct input_event *ev)
{
    struct wye_input_event hev;
    bool swallow = c->forwarding;     /* unmapped input never leaks while forwarding */
    if (translate(ev, &hev)) {
        swallow = c->sink(c->ctx, &hev);
    } else if (ev->type == EV_SYN || ev->type == EV_MSC) {
        swallow = false;
    }
    if (d->grabbed && !swallow) {
        reinject(c, d, ev);
    }
}

void ev_handle(struct ev_capture *c, struct ev_device *d, const struct input_event *ev)
{
    if (ev->type == EV_REL && (ev->code == REL_WHEEL_HI_RES || ev->code == REL_HWHEEL_HI_RES)) {
        return; /* the lo-res wheel events carry the same motion */
    }
    if (d->is_touchpad && d->grabbed && (ev->type == EV_ABS || ev->type == EV_SYN ||
                                         (ev->type == EV_KEY && ev->code >= BTN_DIGI))) {
        int dx = 0, dy = 0, wheel = 0, pan = 0;
        tp_event(&d->tp, ev, &dx, &dy, &wheel, &pan);
        struct input_event rel = {.type = EV_REL};
        int codes[] = {REL_X, REL_Y, REL_WHEEL, REL_HWHEEL}, vals[] = {dx, dy, wheel, pan};
        for (int i = 0; i < 4; ++i) {
            if (vals[i]) {
                rel.code = (uint16_t)codes[i];
                rel.value = vals[i];
                dispatch(c, d, &rel);
            }
        }
        return;
    }
    dispatch(c, d, ev);
}
