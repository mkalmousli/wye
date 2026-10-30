/*
 * Touchpad -> relative pointer while forwarding (when the desktop no longer
 * sees the pad): one finger moves, two fingers scroll. The baseline resets
 * whenever the finger count changes, so the pointer never jumps.
 */
#include "evdev.h"

#include <string.h>
#include <sys/ioctl.h>

static int units_per_mm(int fd, int axis, int fallback_mm)
{
    struct input_absinfo info = {0};
    if (ioctl(fd, EVIOCGABS(axis), &info) != 0) {
        return 10;
    }
    if (info.resolution > 0) {
        return info.resolution;
    }
    int u = (info.maximum - info.minimum) / fallback_mm;
    return u > 0 ? u : 10;
}

void tp_init(struct touchpad *tp, int fd)
{
    struct input_absinfo slot;
    memset(tp, 0, sizeof(*tp));
    tp->multitouch = ioctl(fd, EVIOCGABS(ABS_MT_SLOT), &slot) == 0;
    tp->res_x = units_per_mm(fd, tp->multitouch ? ABS_MT_POSITION_X : ABS_X, 100);
    tp->res_y = units_per_mm(fd, tp->multitouch ? ABS_MT_POSITION_Y : ABS_Y, 60);
}

static int scaled(int delta, int per, int *rem)
{
    long long v = (long long)delta + *rem;
    int out = (int)(v / per);
    *rem = (int)(v % per);
    return out;
}

static void finish_frame(struct touchpad *tp, int *dx, int *dy, int *wheel, int *pan)
{
    int fingers = 0, cx = 0, cy = 0;
    for (int i = 0; i < (tp->multitouch ? TP_SLOTS : 1); ++i) {
        if (tp->active[i]) {
            fingers++;
            cx += tp->x[i];
            cy += tp->y[i];
        }
    }
    if (fingers) {
        cx /= fingers;
        cy /= fingers;
    }
    if (fingers && fingers == tp->fingers_prev && !tp->changed) {
        /* ~10 pointer counts per millimetre of finger travel. */
        if (fingers == 1) {
            *dx = scaled((cx - tp->cx_prev) * 10, tp->res_x, &tp->rem_x);
            *dy = scaled((cy - tp->cy_prev) * 10, tp->res_y, &tp->rem_y);
        } else if (fingers == 2) {
            *pan = scaled(cx - tp->cx_prev, tp->res_x * 5, &tp->scroll_x);
            *wheel = scaled(tp->cy_prev - cy, tp->res_y * 5, &tp->scroll_y);
        }
    } else {
        tp->rem_x = tp->rem_y = tp->scroll_x = tp->scroll_y = 0;
    }
    tp->fingers_prev = fingers;
    tp->cx_prev = cx;
    tp->cy_prev = cy;
    tp->changed = false;
}

bool tp_event(struct touchpad *tp, const struct input_event *ev, int *dx, int *dy, int *wheel, int *pan)
{
    int s = tp->multitouch ? tp->slot : 0;
    if (ev->type == EV_ABS && ev->code == ABS_MT_SLOT) {
        tp->slot = ev->value >= 0 && ev->value < TP_SLOTS ? ev->value : 0;
    } else if (ev->type == EV_ABS && ev->code == ABS_MT_TRACKING_ID) {
        tp->active[s] = ev->value >= 0;
        tp->changed = true;
    } else if (ev->type == EV_ABS && ev->code == (tp->multitouch ? ABS_MT_POSITION_X : ABS_X)) {
        tp->x[s] = ev->value;
    } else if (ev->type == EV_ABS && ev->code == (tp->multitouch ? ABS_MT_POSITION_Y : ABS_Y)) {
        tp->y[s] = ev->value;
    } else if (ev->type == EV_KEY && ev->code == BTN_TOUCH && !tp->multitouch) {
        tp->active[0] = ev->value != 0;
        tp->changed = true;
    } else if (ev->type == EV_SYN && ev->code == SYN_REPORT) {
        finish_frame(tp, dx, dy, wheel, pan);
    } else if (ev->type == EV_SYN && ev->code == SYN_DROPPED) {
        memset(tp->active, 0, sizeof(tp->active));
        tp->changed = true;
    }
    return true;
}
