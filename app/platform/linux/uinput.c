/* Virtual keyboard + pointer used to re-inject local input. */
#define _DEFAULT_SOURCE
#include "evdev.h"

#include <fcntl.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

void ui_emit(int fd, uint16_t type, uint16_t code, int32_t value)
{
    struct input_event ev = {.type = type, .code = code, .value = value};
    if (fd >= 0 && write(fd, &ev, sizeof(ev)) != (ssize_t)sizeof(ev)) {
        /* Dropped event: the desktop is not reading; nothing better to do. */
    }
}

static int create(const char *name, bool keyboard)
{
    int fd = open("/dev/uinput", O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
    if (keyboard) {
        ioctl(fd, UI_SET_EVBIT, EV_MSC);
        ioctl(fd, UI_SET_EVBIT, EV_LED);
        ioctl(fd, UI_SET_EVBIT, EV_REP);
        ioctl(fd, UI_SET_MSCBIT, MSC_SCAN);
        for (int k = 1; k < BTN_MISC; ++k) {
            ioctl(fd, UI_SET_KEYBIT, k);
        }
        for (int k = KEY_OK; k < KEY_MAX; ++k) {
            ioctl(fd, UI_SET_KEYBIT, k);
        }
        ioctl(fd, UI_SET_LEDBIT, LED_NUML);
        ioctl(fd, UI_SET_LEDBIT, LED_CAPSL);
        ioctl(fd, UI_SET_LEDBIT, LED_SCROLLL);
    } else {
        ioctl(fd, UI_SET_EVBIT, EV_REL);
        ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_POINTER);
        for (int b = BTN_LEFT; b <= BTN_TASK; ++b) {
            ioctl(fd, UI_SET_KEYBIT, b);
        }
        for (int r = 0; r <= REL_MAX; ++r) {
            ioctl(fd, UI_SET_RELBIT, r);
        }
    }
    struct uinput_setup setup = {.id = {BUS_VIRTUAL, 0x1209, keyboard ? 0x00f1 : 0x00f2, 1}};
    snprintf(setup.name, sizeof(setup.name), "%s", name);
    if (ioctl(fd, UI_DEV_SETUP, &setup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

bool ui_create(struct ev_capture *c)
{
    c->ui_kbd = create("Wye virtual keyboard", true);
    c->ui_ptr = create("Wye virtual pointer", false);
    if (c->ui_kbd < 0 || c->ui_ptr < 0) {
        ui_destroy(c);
        return false;
    }
    usleep(100 * 1000); /* let the desktop pick up the new devices */
    return true;
}

void ui_destroy(struct ev_capture *c)
{
    int *fds[] = {&c->ui_kbd, &c->ui_ptr};
    for (int i = 0; i < 2; ++i) {
        if (*fds[i] >= 0) {
            ioctl(*fds[i], UI_DEV_DESTROY);
            close(*fds[i]);
            *fds[i] = -1;
        }
    }
}

/* The desktop sets lock LEDs on our virtual keyboard; mirror them physically. */
void ui_read_leds(struct ev_capture *c)
{
    struct input_event ev;
    uint8_t leds = c->local_leds;
    while (read(c->ui_kbd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
        if (ev.type == EV_LED && ev.code <= LED_SCROLLL) {
            uint8_t bit = (uint8_t)(1u << ev.code);
            leds = ev.value ? (uint8_t)(leds | bit) : (uint8_t)(leds & ~bit);
        }
    }
    if (leds != c->local_leds) {
        c->local_leds = leds;
        c->leds_dirty = true;
    }
}
