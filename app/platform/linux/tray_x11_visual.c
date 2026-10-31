/* What the X11 tray draws: the icon, the menu popup, and the visual the panel asks for. */
#include "tray_x11.h"

#include <stdlib.h>

void x11_paint_icon(void)
{
    if (x11.icon && x11.rgba) {
        x11_put_pixels(x11.icon, &x11.icon_surface, x11.rgba, x11.rgba_size, x11.rgba_size);
    }
}

/* The tray says which visual it wants; 32-bit ones allow a really transparent background. */
struct surface x11_tray_surface(Window owner)
{
    struct surface s = {DefaultVisual(x11.dpy, x11.screen), DefaultDepth(x11.dpy, x11.screen), false};
    Atom prop = XInternAtom(x11.dpy, "_NET_SYSTEM_TRAY_VISUAL", False), type;
    int format;
    unsigned long count, after;
    unsigned char *data = NULL;
    if (XGetWindowProperty(x11.dpy, owner, prop, 0, 1, False, XA_VISUALID, &type, &format, &count, &after, &data) ==
            Success && data && count == 1) {
        XVisualInfo want = {.visualid = (VisualID)*(unsigned long *)data}, *found;
        int n = 0;
        found = XGetVisualInfo(x11.dpy, VisualIDMask, &want, &n);
        if (found && n > 0 && found->class == TrueColor) {
            s.visual = found->visual;
            s.depth = found->depth;
            s.has_alpha = found->depth == 32;
        }
        if (found) {
            XFree(found);
        }
    }
    if (data) {
        XFree(data);
    }
    return s;
}

const struct surface *x11_screen_surface(void)
{
    static struct surface s;
    s = (struct surface){DefaultVisual(x11.dpy, x11.screen), DefaultDepth(x11.dpy, x11.screen), false};
    return &s;
}

void x11_paint_popup(const struct plat_tray_item *items, int count, int hover)
{
    int w = 0, h = 0;
    uint8_t *rgba = x11.painter ? x11.painter(items, count, hover, &w, &h) : NULL;
    if (rgba) {
        if (x11.popup && w == x11.popup_w && h == x11.popup_h) {
            x11_put_pixels(x11.popup, x11_screen_surface(), rgba, w, h);
        }
        free(rgba);
    }
}
