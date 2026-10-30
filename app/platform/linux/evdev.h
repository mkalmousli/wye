/*
 * Linux capture internals.
 *   Keyboards: always grabbed; events not swallowed are re-injected through
 *     a uinput keyboard (so the hotkey can be hidden from the desktop).
 *   Pointers: grabbed only in relative forwarding mode; otherwise the desktop
 *     reads them directly (native acceleration, gestures, touchpad settings).
 */
#ifndef WYE_EVDEV_H
#define WYE_EVDEV_H

#include "plat_input.h"
#include "plat_thread.h"

#include <linux/input.h>

#define EV_MAX_DEVICES 64
#define TP_SLOTS 16

struct touchpad {
    bool multitouch;
    int slot, res_x, res_y;
    bool active[TP_SLOTS];
    int x[TP_SLOTS], y[TP_SLOTS];
    int fingers_prev, cx_prev, cy_prev;
    int rem_x, rem_y, scroll_x, scroll_y;
    bool changed;
};

struct ev_device {
    int fd;
    char path[64];
    char name[128];
    bool keyboard, pointer, is_touchpad, has_leds;
    bool grabbed;
    struct touchpad tp;
};

struct ev_capture {
    wye_input_sink sink;
    void *ctx;
    struct ev_device dev[EV_MAX_DEVICES];
    int count;
    int inotify_fd;
    plat_event *wake;
    int ui_kbd, ui_ptr;              /* uinput outputs */
    volatile bool forwarding, stop, rescan;
    volatile uint8_t target_leds, local_leds;
    volatile bool leds_dirty;
    volatile uint64_t heartbeat;
    volatile enum plat_input_status status;
    plat_thread *thread, *watchdog;
};

/* capture.c */
void ev_thread_main(void *arg);
/* evdev_devices.c */
void ev_scan(struct ev_capture *c);
void ev_remove(struct ev_capture *c, int index);
void ev_apply_grabs(struct ev_capture *c);
void ev_release_all(struct ev_capture *c);
void ev_write_leds(struct ev_capture *c, uint8_t leds);
/* evdev_events.c */
void ev_handle(struct ev_capture *c, struct ev_device *d, const struct input_event *ev);
/* evdev_keymap.c */
uint16_t ev_key_to_hid(uint16_t code);
uint16_t ev_key_to_consumer(uint16_t code);
uint8_t ev_key_to_system(uint16_t code);
/* uinput.c */
bool ui_create(struct ev_capture *c);
void ui_destroy(struct ev_capture *c);
void ui_emit(int fd, uint16_t type, uint16_t code, int32_t value);
void ui_read_leds(struct ev_capture *c);
/* touchpad.c */
void tp_init(struct touchpad *tp, int fd);
/* Returns true if consumed; fills relative motion/scroll on SYN frames. */
bool tp_event(struct touchpad *tp, const struct input_event *ev, int *dx, int *dy, int *wheel, int *pan);

#endif
