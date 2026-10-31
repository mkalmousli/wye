/* X11 tray events: errors from the panel, the icon window, and the queue of chosen items. */
#include "tray_x11.h"

#include "plat_thread.h"

#include <string.h>

/* A panel going away can make any of our requests fail: that must not end the program, which is
   what Xlib does by default. Errors on other connections go where they went before. */
int x11_on_x_error(Display *dpy, XErrorEvent *ev)
{
    return dpy == x11.dpy ? 0 : x11.prev_errors ? x11.prev_errors(dpy, ev) : 0;
}

void x11_push(int id)
{
    if (x11.queued < QUEUE) {
        x11.queue[x11.queued++] = id;
    }
}

static void on_icon_event(const XEvent *ev)
{
    switch (ev->type) {
    case Expose:
        if (ev->xexpose.count == 0) {
            x11_paint_icon();
        }
        break;
    case ConfigureNotify:
        if (ev->xconfigure.width >= 8 && ev->xconfigure.width != x11.size) {
            x11.size = ev->xconfigure.width; /* the UI gives us a new picture at this size */
        }
        break;
    case DestroyNotify:
        x11.icon = 0;
        x11.docked = false;
        x11.owner = 0;
        break;
    case EnterNotify:
        x11.hovering = true;
        x11.tip_at = plat_now_ms() + TIP_DELAY_MS;
        break;
    case LeaveNotify:
        x11.hovering = false;
        if (x11.tip_open) {
            x11_close_popup();
        }
        break;
    case ButtonPress:
        x11.hovering = false;
        x11_close_popup();
        if (ev->xbutton.button == BUTTON_LEFT) {
            x11_push(PLAT_TRAY_ACTIVATE);
        } else if (ev->xbutton.button == BUTTON_RIGHT) {
            x11_open_menu();
        }
        break;
    default:
        break;
    }
}

void x11_pump(uint64_t now)
{
    while (XPending(x11.dpy) > 0) {
        XEvent ev;
        XNextEvent(x11.dpy, &ev);
        if (x11.icon && ev.xany.window == x11.icon) {
            on_icon_event(&ev);
        } else if (x11.popup && ev.xany.window == x11.popup) {
            x11_on_popup_event(&ev);
        }
    }
    if (x11.hovering && !x11.popup && x11.tip_at && now >= x11.tip_at) {
        x11.tip_at = 0;
        x11_open_tip();
    }
}

int x11_take_choice(void)
{
    if (!x11.queued) {
        return PLAT_TRAY_NONE;
    }
    int id = x11.queue[0];
    memmove(x11.queue, x11.queue + 1, (size_t)--x11.queued * sizeof(x11.queue[0]));
    return id;
}
