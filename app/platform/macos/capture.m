/* macOS: keyboard and mouse capture through a CGEventTap on its own run loop. */
#include "capture_internal.h"
#include "plat_thread.h"

struct mac_capture g_mac_cap;
static plat_thread *g_thread;

static void capture_thread(void *ready_arg)
{
    plat_event *ready = ready_arg;
    CFMachPortRef tap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap,
                                         kCGEventTapOptionDefault, mac_event_mask(),
                                         mac_tap_callback, NULL);
    if (!tap) { /* no Input Monitoring / Accessibility permission yet */
        g_mac_cap.status = PLAT_INPUT_NO_PERMISSION;
        plat_event_signal(ready);
        return;
    }
    CFRunLoopSourceRef src = CFMachPortCreateRunLoopSource(NULL, tap, 0);
    g_mac_cap.tap = tap;
    g_mac_cap.loop = CFRunLoopGetCurrent();
    CFRunLoopAddSource(g_mac_cap.loop, src, kCFRunLoopCommonModes);
    CGEventTapEnable(tap, true);
    g_mac_cap.status = PLAT_INPUT_RUNNING;
    plat_event_signal(ready);
    CFRunLoopRun();
    CFRunLoopRemoveSource(g_mac_cap.loop, src, kCFRunLoopCommonModes);
    CFRelease(src);
    CFRelease(tap);
}

bool plat_input_start(wye_input_sink sink, void *ctx, char *err, size_t errlen)
{
    if (g_mac_cap.status == PLAT_INPUT_RUNNING) {
        return true;
    }
    g_mac_cap.sink = sink;
    g_mac_cap.ctx = ctx;
    plat_event *ready = plat_event_new();
    g_mac_cap.status = PLAT_INPUT_STOPPED;
    g_thread = plat_thread_start(capture_thread, ready);
    bool up = plat_event_wait(ready, 5000) && g_mac_cap.status == PLAT_INPUT_RUNNING;
    plat_event_free(ready);
    if (!up) {
        if (g_mac_cap.status == PLAT_INPUT_NO_PERMISSION) {
            snprintf(err, errlen, "allow Wye under Privacy & Security (Input Monitoring and Accessibility)");
        } else {
            snprintf(err, errlen, "could not start keyboard and mouse capture");
        }
        enum plat_input_status why = g_mac_cap.status;
        plat_input_stop();
        g_mac_cap.status = why == PLAT_INPUT_NO_PERMISSION ? why : PLAT_INPUT_FAILED;
        return false;
    }
    return true;
}

void plat_input_stop(void)
{
    if (g_thread) {
        g_mac_cap.forwarding = 0;
        mac_apply_forwarding();
        if (g_mac_cap.loop) {
            CFRunLoopStop(g_mac_cap.loop);
        }
        plat_thread_join(g_thread);
        g_thread = NULL;
    }
    g_mac_cap.loop = NULL;
    g_mac_cap.tap = NULL;
    g_mac_cap.status = PLAT_INPUT_STOPPED;
}

enum plat_input_status plat_input_status(void)
{
    return g_mac_cap.status;
}

void plat_input_set_forwarding(bool forwarding)
{
    g_mac_cap.forwarding = forwarding;
    mac_apply_forwarding();
}

void plat_input_set_pointer_local(bool local)
{
    g_mac_cap.pointer_local = local;
    mac_apply_forwarding();
}

/* macOS keyboards have no lock-key LEDs that apps can drive, so this does nothing. */
void plat_input_set_leds(uint8_t leds)
{
    (void)leds;
}
