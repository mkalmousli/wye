/* The X11 tray's popup window: placed beside the icon, closed by a click elsewhere; the tooltip uses it too. */
#include "tray_x11.h"

#include <stdlib.h>
#include <string.h>

int x11_row_at(int y)
{
    int top = 0;
    for (int i = 0; i < x11.item_count; ++i) {
        int h = x11.items[i].label[0] ? PLAT_TRAY_ROW_H : PLAT_TRAY_SEPARATOR_H;
        if (y >= top && y < top + h) {
            return x11.items[i].label[0] && x11.items[i].enabled ? i : -1;
        }
        top += h;
    }
    return -1;
}

void x11_close_popup(void)
{
    if (x11.popup) {
        if (x11.menu_open) {
            XUngrabPointer(x11.dpy, CurrentTime);
        }
        XDestroyWindow(x11.dpy, x11.popup);
        XFlush(x11.dpy);
        x11.popup = 0;
    }
    x11.menu_open = x11.tip_open = false;
}

/* Opens next to the icon, inside the screen, on whichever side has room. */
void x11_open_popup(const struct plat_tray_item *items, int count)
{
    int w = 0, h = 0, x = 0, y = 0;
    Window root = RootWindow(x11.dpy, x11.screen), child;
    uint8_t *rgba = x11.painter ? x11.painter(items, count, -1, &w, &h) : NULL;
    if (!rgba) {
        return;
    }
    XTranslateCoordinates(x11.dpy, x11.icon, root, 0, 0, &x, &y, &child);
    int sw = DisplayWidth(x11.dpy, x11.screen), sh = DisplayHeight(x11.dpy, x11.screen);
    x = x + w > sw ? sw - w : x;
    y = y + x11.size + h > sh ? y - h : y + x11.size;
    y = y < 0 ? 0 : y;
    XSetWindowAttributes a;
    memset(&a, 0, sizeof(a));
    a.override_redirect = True;
    a.save_under = True;
    a.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | LeaveWindowMask;
    x11.popup = XCreateWindow(x11.dpy, root, x, y, (unsigned)w, (unsigned)h, 0, CopyFromParent, InputOutput,
                              CopyFromParent, CWOverrideRedirect | CWSaveUnder | CWEventMask, &a);
    x11.popup_w = w;
    x11.popup_h = h;
    XMapRaised(x11.dpy, x11.popup);
    XSync(x11.dpy, False);
    x11_put_pixels(x11.popup, x11_screen_surface(), rgba, w, h);
    free(rgba);
}

void x11_open_tip(void)
{
    if (!x11.tooltip[0] || x11.popup) {
        return;
    }
    memcpy(x11.tip.label, x11.tooltip, sizeof(x11.tip.label));
    x11.tip.enabled = true;
    x11_open_popup(&x11.tip, 1);
    x11.tip_open = x11.popup != 0;
}
