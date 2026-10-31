/* Puts RGBA pixels on an X window, converted to the visual's own pixel layout. */
#include "tray_x11.h"

#include <stdlib.h>

static unsigned long channel(uint8_t value, unsigned long mask)
{
    if (!mask) {
        return 0;
    }
    int shift = 0, bits = 0;
    while (!((mask >> shift) & 1u)) {
        shift++;
    }
    while ((mask >> (shift + bits)) & 1u) {
        bits++;
    }
    return ((unsigned long)value >> (bits < 8 ? 8 - bits : 0)) << shift;
}

/* Without an alpha channel the pixels are mixed with what is behind them. */
void x11_put_pixels(Window w, const struct surface *s, const uint8_t *rgba, int width, int height)
{
    XImage *behind = NULL;
    if (!s->has_alpha) {
        XClearWindow(x11.dpy, w);
        XSync(x11.dpy, False);
        behind = XGetImage(x11.dpy, w, 0, 0, (unsigned)width, (unsigned)height, AllPlanes, ZPixmap);
    }
    char *data = malloc((size_t)width * (size_t)height * 4);
    XImage *img = data ? XCreateImage(x11.dpy, s->visual, (unsigned)s->depth, ZPixmap, 0, data, (unsigned)width,
                                      (unsigned)height, 32, 0) : NULL;
    if (!img) {
        free(data);
        if (behind) {
            XDestroyImage(behind);
        }
        return;
    }
    unsigned long rm = s->visual->red_mask, gm = s->visual->green_mask, bm = s->visual->blue_mask;
    unsigned long am = s->has_alpha ? 0xffffffffUL & ~(rm | gm | bm) : 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const uint8_t *p = rgba + ((size_t)y * (size_t)width + (size_t)x) * 4;
            unsigned a = p[3], r, gr, b;
            if (s->has_alpha) { /* premultiplied, as compositors expect */
                r = p[0] * a / 255u;
                gr = p[1] * a / 255u;
                b = p[2] * a / 255u;
            } else {
                unsigned long under = behind ? XGetPixel(behind, x, y) : 0;
                unsigned br = behind ? (unsigned)((under & rm) * 255u / (rm ? rm : 1u)) : 40u;
                unsigned bg = behind ? (unsigned)((under & gm) * 255u / (gm ? gm : 1u)) : 40u;
                unsigned bb = behind ? (unsigned)((under & bm) * 255u / (bm ? bm : 1u)) : 40u;
                r = (p[0] * a + br * (255u - a)) / 255u;
                gr = (p[1] * a + bg * (255u - a)) / 255u;
                b = (p[2] * a + bb * (255u - a)) / 255u;
            }
            XPutPixel(img, x, y, channel((uint8_t)r, rm) | channel((uint8_t)gr, gm) | channel((uint8_t)b, bm) |
                                     channel((uint8_t)a, am));
        }
    }
    GC gc = XCreateGC(x11.dpy, w, 0, NULL);
    XPutImage(x11.dpy, w, gc, img, 0, 0, 0, 0, (unsigned)width, (unsigned)height);
    XFreeGC(x11.dpy, gc);
    XDestroyImage(img); /* frees data too */
    if (behind) {
        XDestroyImage(behind);
    }
    XFlush(x11.dpy);
}
