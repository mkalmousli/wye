/* plat_input API for Linux, plus the stall watchdog. */
#define _DEFAULT_SOURCE
#include "evdev.h"
#include "log.h"

#include <stdio.h>
#include <sys/inotify.h>
#include <unistd.h>

#define WATCHDOG_MS 2500u

static struct ev_capture g_cap = {.inotify_fd = -1, .ui_kbd = -1, .ui_ptr = -1};

/* If the capture thread ever stalls, give the keyboard back to the desktop. */
static void watchdog_thread(void *arg)
{
    struct ev_capture *c = arg;
    while (!c->stop) {
        plat_sleep_ms(500);
        if (!c->stop && plat_now_ms() - c->heartbeat > WATCHDOG_MS) {
            LOG_E("input: capture thread stalled; releasing all devices");
            ev_release_all(c);
            c->status = PLAT_INPUT_FAILED;
            return;
        }
    }
}

bool plat_input_start(wye_input_sink sink, void *ctx, char *err, size_t errlen)
{
    struct ev_capture *c = &g_cap;
    c->sink = sink;
    c->ctx = ctx;
    c->wake = plat_event_new();
    if (!ui_create(c)) {
        snprintf(err, errlen, "cannot open /dev/uinput (permission setup needed)");
        c->status = PLAT_INPUT_NO_PERMISSION;
        return false;
    }
    c->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    inotify_add_watch(c->inotify_fd, "/dev/input", IN_CREATE | IN_ATTRIB);
    ev_scan(c);
    c->heartbeat = plat_now_ms();
    c->status = c->count ? PLAT_INPUT_RUNNING : PLAT_INPUT_NO_PERMISSION;
    c->thread = plat_thread_start(ev_thread_main, c);
    c->watchdog = plat_thread_start(watchdog_thread, c);
    return true;
}

void plat_input_stop(void)
{
    struct ev_capture *c = &g_cap;
    if (!c->thread) {
        return;
    }
    c->stop = true;
    plat_event_signal(c->wake);
    plat_thread_join(c->thread);
    plat_thread_join(c->watchdog);
    c->thread = c->watchdog = NULL;
    ev_release_all(c);
    ui_destroy(c);
    close(c->inotify_fd);
    c->status = PLAT_INPUT_STOPPED;
}

enum plat_input_status plat_input_status(void) { return g_cap.status; }

void plat_input_set_forwarding(bool forwarding)
{
    g_cap.forwarding = forwarding;
    g_cap.leds_dirty = true;
    if (g_cap.wake) {
        plat_event_signal(g_cap.wake); /* grabs change on the capture thread */
    }
}

void plat_input_set_pointer_local(bool local)
{
    (void)local; /* events the sink does not swallow are re-injected here anyway */
}

void plat_input_set_leds(uint8_t leds)
{
    if (g_cap.target_leds != leds) {
        g_cap.target_leds = leds;
        g_cap.leds_dirty = true;
        plat_event_signal(g_cap.wake);
    }
}
