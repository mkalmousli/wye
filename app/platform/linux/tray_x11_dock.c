/* Docking into the panel's system tray over XEmbed, and noticing when the tray comes or goes. */
#include "tray_x11.h"

#include <string.h>

void x11_undock(void)
{
    if (x11.icon) {
        XDestroyWindow(x11.dpy, x11.icon);
        x11.icon = 0;
    }
    if (x11.icon_colormap) {
        XFreeColormap(x11.dpy, x11.icon_colormap);
        x11.icon_colormap = 0;
    }
    x11.docked = false;
}

void x11_dock(Window owner)
{
    x11_undock();
    x11.owner = owner;
    if (!owner) {
        return;
    }
    x11.icon_surface = x11_tray_surface(owner);
    Window root = RootWindow(x11.dpy, x11.screen);
    XSetWindowAttributes a;
    memset(&a, 0, sizeof(a));
    unsigned long mask = CWEventMask | CWColormap | CWBorderPixel;
    x11.icon_colormap = XCreateColormap(x11.dpy, root, x11.icon_surface.visual, AllocNone);
    a.colormap = x11.icon_colormap;
    a.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask | EnterWindowMask | LeaveWindowMask;
    if (x11.icon_surface.has_alpha) {
        mask |= CWBackPixel;
    } else {
        mask |= CWBackPixmap;
        a.background_pixmap = ParentRelative; /* the panel shows through */
    }
    x11.size = x11.size ? x11.size : 22;
    x11.icon = XCreateWindow(x11.dpy, root, 0, 0, (unsigned)x11.size, (unsigned)x11.size, 0, x11.icon_surface.depth,
                             InputOutput, x11.icon_surface.visual, mask, &a);
    XStoreName(x11.dpy, x11.icon, "Wye");
    XClassHint hint = {(char *)"wye", (char *)"Wye"};
    XSetClassHint(x11.dpy, x11.icon, &hint);
    unsigned long info[2] = {0, XEMBED_MAPPED};
    Atom xembed = XInternAtom(x11.dpy, "_XEMBED_INFO", False);
    XChangeProperty(x11.dpy, x11.icon, xembed, xembed, 32, PropModeReplace, (unsigned char *)info, 2);

    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = owner;
    ev.xclient.message_type = XInternAtom(x11.dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = OPCODE_REQUEST_DOCK;
    ev.xclient.data.l[2] = (long)x11.icon;
    XSendEvent(x11.dpy, owner, False, NoEventMask, &ev);
    XMapRaised(x11.dpy, x11.icon); /* XEmbed: the panel only shows a docked icon once it is mapped */
    XFlush(x11.dpy);
    x11.docked = true;
}

/* The tray can appear after us, or restart: keep looking. */
void x11_check_owner(uint64_t now)
{
    if (now < x11.next_check) {
        return;
    }
    x11.next_check = now + RECHECK_MS;
    Window owner = XGetSelectionOwner(x11.dpy, x11.selection);
    if (owner != x11.owner) {
        x11_dock(owner);
    }
}
