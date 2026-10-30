/* Grabbing devices and writing keyboard LEDs. */
#include "evdev.h"

#include <sys/ioctl.h>

void ev_apply_grabs(struct ev_capture *c)
{
    for (int i = 0; i < c->count; ++i) {
        struct ev_device *d = &c->dev[i];
        bool want = d->keyboard || (d->pointer && c->forwarding);
        if (want != d->grabbed && ioctl(d->fd, EVIOCGRAB, want ? 1 : 0) == 0) {
            d->grabbed = want;
        }
    }
}

void ev_release_all(struct ev_capture *c)
{
    for (int i = 0; i < c->count; ++i) {
        ioctl(c->dev[i].fd, EVIOCGRAB, 0);
        c->dev[i].grabbed = false;
    }
}

void ev_write_leds(struct ev_capture *c, uint8_t leds)
{
    static const uint16_t codes[] = {LED_NUML, LED_CAPSL, LED_SCROLLL};
    for (int i = 0; i < c->count; ++i) {
        for (int b = 0; b < 3 && c->dev[i].has_leds; ++b) {
            ui_emit(c->dev[i].fd, EV_LED, codes[b], (leds >> b) & 1);
        }
        if (c->dev[i].has_leds) {
            ui_emit(c->dev[i].fd, EV_SYN, SYN_REPORT, 0);
        }
    }
}
