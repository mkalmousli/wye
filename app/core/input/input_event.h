/* OS-neutral input event produced by every platform capture backend. */
#ifndef WYE_INPUT_EVENT_H
#define WYE_INPUT_EVENT_H

#include <stdbool.h>
#include <stdint.h>

enum wye_ev_type {
    WYE_EV_KEY = 1,      /* code: USB HID keyboard usage (page 0x07) */
    WYE_EV_CONSUMER,     /* code: HID consumer usage (page 0x0C) */
    WYE_EV_SYSTEM,       /* code: HID system control usage (1 power, 2 sleep, 3 wake) */
    WYE_EV_BUTTON,       /* code: WYE_BTN_* bit */
    WYE_EV_MOVE,         /* dx, dy: relative device counts */
    WYE_EV_WHEEL,        /* dy: vertical, dx: horizontal; 120 units per notch */
};

struct wye_input_event {
    uint8_t type;
    bool down;
    bool repeat;         /* OS auto-repeat (keys only) */
    uint16_t code;
    int32_t dx, dy;
};

/*
 * Called on the capture thread for every event. Return true to swallow the
 * event (it never reaches the local OS), false to let it through.
 * Must be fast: Windows and macOS disable slow hooks.
 */
typedef bool (*wye_input_sink)(void *ctx, const struct wye_input_event *ev);

#endif
