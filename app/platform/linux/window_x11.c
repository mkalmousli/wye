/* X11 window extras: sticky across workspaces (EWMH) and the global pointer position. */
#include "plat_window.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <string.h>

/* GLFW (built into raylib with its X11 backend) exposes the native handles. */
Display *glfwGetX11Display(void);
Window glfwGetX11Window(void *window);

static void send_state(Display *dpy, Window w, const char *type, long a, long b, long c)
{
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = XInternAtom(dpy, type, False);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = a;
    ev.xclient.data.l[1] = b;
    ev.xclient.data.l[2] = c;
    ev.xclient.data.l[3] = 1; /* source: normal application */
    XSendEvent(dpy, DefaultRootWindow(dpy), False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
}

void plat_window_all_desktops(void *handle, bool on)
{
    Display *dpy = handle ? glfwGetX11Display() : NULL;
    Window w = dpy ? glfwGetX11Window(handle) : 0;
    if (!w) {
        return; /* Wayland-native or no window: nothing to do */
    }
    /* _NET_WM_DESKTOP 0xFFFFFFFF = all desktops; STICKY for WMs that prefer the state. */
    send_state(dpy, w, "_NET_WM_DESKTOP", on ? 0xffffffffL : 0, 1, 0);
    send_state(dpy, w, "_NET_WM_STATE", on ? 1 : 0, (long)XInternAtom(dpy, "_NET_WM_STATE_STICKY", False), 0);
    XFlush(dpy);
}

static long current_desktop(Display *dpy)
{
    Atom type;
    int format;
    unsigned long count, after;
    unsigned char *data = NULL;
    long desk = 0;
    Atom prop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    if (XGetWindowProperty(dpy, DefaultRootWindow(dpy), prop, 0, 1, False, XA_CARDINAL, &type, &format, &count,
                           &after, &data) == Success && data && count == 1) {
        desk = *(long *)data;
    }
    if (data) {
        XFree(data);
    }
    return desk;
}

void plat_window_to_current_desktop(void *handle)
{
    Display *dpy = handle ? glfwGetX11Display() : NULL;
    Window w = dpy ? glfwGetX11Window(handle) : 0;
    if (!w) {
        return;
    }
    send_state(dpy, w, "_NET_WM_STATE", 0, (long)XInternAtom(dpy, "_NET_WM_STATE_STICKY", False), 0);
    send_state(dpy, w, "_NET_WM_DESKTOP", current_desktop(dpy), 1, 0);
    send_state(dpy, w, "_NET_ACTIVE_WINDOW", 1, CurrentTime, 0); /* raise and focus */
    XFlush(dpy);
}

bool plat_cursor_screen_pos(void *handle, int *x, int *y)
{
    Display *dpy = handle ? glfwGetX11Display() : NULL;
    Window root, child;
    int wx, wy;
    unsigned int mask;
    return dpy && XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &child, x, y, &wx, &wy, &mask);
}
