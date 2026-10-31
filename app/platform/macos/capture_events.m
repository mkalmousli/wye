/* macOS: turns one CGEvent into one Wye input event. */
#include "capture_internal.h"

#import <AppKit/AppKit.h>

#define NX_SUBTYPE_AUX_KEYS 8

static uint16_t button_bit(int64_t number)
{
    switch (number) {
    case 0: return WYE_BTN_LEFT;
    case 1: return WYE_BTN_RIGHT;
    case 2: return WYE_BTN_MIDDLE;
    case 3: return WYE_BTN_BACK;
    default: return WYE_BTN_FORWARD;
    }
}

static CGEventFlags modifier_mask(int64_t kvk)
{
    switch (kvk) {
    case 0x38: case 0x3C: return kCGEventFlagMaskShift;
    case 0x3B: case 0x3E: return kCGEventFlagMaskControl;
    case 0x3A: case 0x3D: return kCGEventFlagMaskAlternate;
    case 0x37: case 0x36: return kCGEventFlagMaskCommand;
    case 0x39: return kCGEventFlagMaskAlphaShift;
    default: return 0;
    }
}

static void plain(struct wye_input_event *ev, uint8_t type)
{
    *ev = (struct wye_input_event){.type = type};
}

static bool key_event(CGEventType type, CGEventRef e, struct wye_input_event *ev)
{
    int64_t kvk = CGEventGetIntegerValueField(e, kCGKeyboardEventKeycode);
    plain(ev, WYE_EV_KEY);
    ev->code = mac_keycode_to_hid((uint16_t)kvk);
    if (type == kCGEventFlagsChanged) {
        ev->down = (CGEventGetFlags(e) & modifier_mask(kvk)) != 0;
    } else {
        ev->down = type == kCGEventKeyDown;
        ev->repeat = CGEventGetIntegerValueField(e, kCGKeyboardEventAutorepeat) != 0;
    }
    return ev->code != 0;
}

static bool mouse_event(CGEventType type, CGEventRef e, struct wye_input_event *ev)
{
    switch (type) {
    case kCGEventLeftMouseDown: case kCGEventRightMouseDown: case kCGEventOtherMouseDown:
    case kCGEventLeftMouseUp: case kCGEventRightMouseUp: case kCGEventOtherMouseUp:
        plain(ev, WYE_EV_BUTTON);
        ev->code = button_bit(CGEventGetIntegerValueField(e, kCGMouseEventButtonNumber));
        ev->down = type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown ||
                   type == kCGEventOtherMouseDown;
        return true;
    case kCGEventScrollWheel:
        plain(ev, WYE_EV_WHEEL);
        ev->dx = (int32_t)CGEventGetIntegerValueField(e, kCGScrollWheelEventPointDeltaAxis2);
        ev->dy = (int32_t)CGEventGetIntegerValueField(e, kCGScrollWheelEventPointDeltaAxis1);
        return ev->dx || ev->dy;
    default: /* moves and drags: deltas keep coming while the pointer is frozen */
        plain(ev, WYE_EV_MOVE);
        ev->dx = (int32_t)CGEventGetIntegerValueField(e, kCGMouseEventDeltaX);
        ev->dy = (int32_t)CGEventGetIntegerValueField(e, kCGMouseEventDeltaY);
        return ev->dx || ev->dy;
    }
}

/* Media keys: system-defined events, decoded through NSEvent. */
static bool aux_event(CGEventRef e, struct wye_input_event *ev)
{
    NSEvent *n = [NSEvent eventWithCGEvent:e];
    if (!n || n.subtype != NX_SUBTYPE_AUX_KEYS) {
        return false;
    }
    plain(ev, WYE_EV_CONSUMER);
    ev->code = mac_nxkey_to_consumer((int)((n.data1 & 0xFFFF0000) >> 16));
    ev->down = ((n.data1 & 0xFF00) >> 8) == 0xA;
    return ev->code != 0;
}

bool mac_translate(CGEventType type, CGEventRef e, struct wye_input_event *ev)
{
    switch (type) {
    case kCGEventKeyDown: case kCGEventKeyUp: case kCGEventFlagsChanged:
        return key_event(type, e, ev);
    case NX_SYSDEFINED:
        return aux_event(e, ev);
    default:
        return mouse_event(type, e, ev);
    }
}
