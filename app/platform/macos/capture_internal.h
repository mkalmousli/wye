/* macOS capture internals shared by the event tap and the input API. */
#ifndef WYE_MACOS_CAPTURE_INTERNAL_H
#define WYE_MACOS_CAPTURE_INTERNAL_H

#include "plat_input.h"

#include <ApplicationServices/ApplicationServices.h>
#include <stdint.h>

#define NX_SYSDEFINED ((CGEventType)14) /* media keys: NSSystemDefined */

struct mac_capture {
    wye_input_sink sink;
    void *ctx;
    CFMachPortRef tap;            /* the event tap */
    CFRunLoopRef loop;            /* the capture thread's run loop */
    volatile int forwarding, pointer_local;
    volatile enum plat_input_status status;
    double lock_x, lock_y;        /* pointer parked here while forwarding */
};

extern struct mac_capture g_mac_cap;

/* keymap.c */
uint16_t mac_keycode_to_hid(uint16_t kvk);
uint16_t mac_nxkey_to_consumer(int nxkey);

/* capture_events.m */
bool mac_translate(CGEventType type, CGEventRef e, struct wye_input_event *ev);

/* capture_hooks.m */
CGEventRef mac_tap_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef e, void *user);
CGEventMask mac_event_mask(void);
void mac_apply_forwarding(void);

#endif
