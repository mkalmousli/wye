/* The X11 tray's menu popup: opening it, and the pointer events inside it. */
#include "tray_x11.h"

#include "plat_thread.h"

void x11_open_menu(void)
{
    x11_close_popup();
    if (!x11.item_count) {
        return;
    }
    x11_open_popup(x11.items, x11.item_count);
    if (!x11.popup) {
        return;
    }
    x11.menu_open = true;
    x11.hover_row = -1;
    x11.menu_opened = plat_now_ms();
    /* A click anywhere else closes it. */
    XGrabPointer(x11.dpy, x11.popup, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync,
                 GrabModeAsync, None, None, CurrentTime);
    XFlush(x11.dpy);
}

void x11_on_popup_event(const XEvent *ev)
{
    if (!x11.menu_open) {
        if (x11.tip_open && ev->type == Expose && ev->xexpose.count == 0) {
            x11_paint_popup(&x11.tip, 1, -1);
        }
        return;
    }
    if (ev->type == Expose && ev->xexpose.count == 0) {
        x11_paint_popup(x11.items, x11.item_count, x11.hover_row);
    } else if (ev->type == MotionNotify) {
        bool inside = ev->xmotion.x >= 0 && ev->xmotion.x < x11.popup_w;
        int row = inside ? x11_row_at(ev->xmotion.y) : -1;
        if (row != x11.hover_row) {
            x11.hover_row = row;
            x11_paint_popup(x11.items, x11.item_count, row);
        }
    } else if (ev->type == ButtonPress) {
        const XButtonEvent *b = &ev->xbutton;
        if (b->x < 0 || b->y < 0 || b->x >= x11.popup_w || b->y >= x11.popup_h) {
            x11_close_popup();
        }
    } else if (ev->type == ButtonRelease && plat_now_ms() - x11.menu_opened >= OPEN_GUARD_MS) {
        const XButtonEvent *b = &ev->xbutton;
        int row = b->x >= 0 && b->x < x11.popup_w ? x11_row_at(b->y) : -1;
        if (row >= 0) {
            x11_push(x11.items[row].id);
            x11_close_popup();
        }
    }
}
