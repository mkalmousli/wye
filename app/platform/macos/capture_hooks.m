/* macOS: the event tap callback, the tap's event mask, and the pointer freeze used while forwarding. */
#include "capture_internal.h"

#include <ApplicationServices/ApplicationServices.h>

CGEventRef mac_tap_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef e, void *user)
{
    (void)proxy;
    (void)user;
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        CGEventTapEnable((CFMachPortRef)g_mac_cap.tap, true); /* macOS turns slow taps off */
        return e;
    }
    struct wye_input_event ev;
    if (!mac_translate(type, e, &ev) || !g_mac_cap.sink) {
        return e;
    }
    return g_mac_cap.sink(g_mac_cap.ctx, &ev) ? NULL : e; /* NULL swallows the event */
}

CGEventMask mac_event_mask(void)
{
    CGEventType kinds[] = {
        kCGEventKeyDown, kCGEventKeyUp, kCGEventFlagsChanged, kCGEventLeftMouseDown,
        kCGEventLeftMouseUp, kCGEventRightMouseDown, kCGEventRightMouseUp, kCGEventOtherMouseDown,
        kCGEventOtherMouseUp, kCGEventMouseMoved, kCGEventLeftMouseDragged,
        kCGEventRightMouseDragged, kCGEventOtherMouseDragged, kCGEventScrollWheel, NX_SYSDEFINED,
    };
    CGEventMask m = 0;
    for (size_t i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++) {
        m |= CGEventMaskBit(kinds[i]);
    }
    return m;
}

/* Forwarding freezes the pointer where it is and warps it to the screen centre. */
void mac_apply_forwarding(void)
{
    bool freeze = g_mac_cap.forwarding && !g_mac_cap.pointer_local;
    if (freeze) {
        CGRect r = CGDisplayBounds(CGMainDisplayID());
        CGPoint c = CGPointMake(CGRectGetMidX(r), CGRectGetMidY(r));
        g_mac_cap.lock_x = c.x;
        g_mac_cap.lock_y = c.y;
        CGWarpMouseCursorPosition(c);
    }
    CGAssociateMouseAndMouseCursorPosition(!freeze);
}
