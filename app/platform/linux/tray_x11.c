/*
 * System tray on X11, using only Xlib: the freedesktop "system tray" protocol
 * (XEmbed). KDE and others that moved on still offer it through a bridge; plain
 * GNOME has no tray without an extension. The menu is an unmanaged popup window
 * showing what the painter drew. This uses its own X connection.
 */
#include "plat_tray.h"

#include "plat_thread.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPCODE_REQUEST_DOCK 0
#define XEMBED_MAPPED 1u
#define RECHECK_MS 2000u
#define TIP_DELAY_MS 500u
#define OPEN_GUARD_MS 250u  /* the release of the click that opened the menu is not a choice */
#define BUTTON_LEFT 1u
#define BUTTON_RIGHT 3u
#define QUEUE 8

struct surface {  /* how to turn RGBA into this window's pixels */
    Visual *visual;
    int depth;
    bool has_alpha;
};

static struct {
    Display *dpy;
    int screen;
    plat_tray_painter painter;
    int (*prev_errors)(Display *, XErrorEvent *);
    Window owner, icon, popup;
    struct surface icon_surface;
    Colormap icon_colormap;
    Atom selection;
    int size;
    uint8_t *rgba;
    int rgba_size;
    bool docked;
    uint64_t next_check;
    struct plat_tray_item items[PLAT_TRAY_ITEMS];
    int item_count;
    char tooltip[PLAT_TRAY_LABEL];
    struct plat_tray_item tip;
    bool menu_open, tip_open, hovering;
    int hover_row, popup_w, popup_h;
    uint64_t menu_opened, tip_at;
    int queue[QUEUE], queued;
} g;

/* A panel going away can make any of our requests fail: that must not end the program, which is
   what Xlib does by default. Errors on other connections go where they went before. */
static int on_x_error(Display *dpy, XErrorEvent *ev)
{
    return dpy == g.dpy ? 0 : g.prev_errors ? g.prev_errors(dpy, ev) : 0;
}

static void push(int id)
{
    if (g.queued < QUEUE) {
        g.queue[g.queued++] = id;
    }
}

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

/* Puts RGBA pixels on a window. Without an alpha channel they are mixed with what is behind. */
static void put_pixels(Window w, const struct surface *s, const uint8_t *rgba, int width, int height)
{
    XImage *behind = NULL;
    if (!s->has_alpha) {
        XClearWindow(g.dpy, w);
        XSync(g.dpy, False);
        behind = XGetImage(g.dpy, w, 0, 0, (unsigned)width, (unsigned)height, AllPlanes, ZPixmap);
    }
    char *data = malloc((size_t)width * (size_t)height * 4);
    XImage *img = data ? XCreateImage(g.dpy, s->visual, (unsigned)s->depth, ZPixmap, 0, data, (unsigned)width,
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
    GC gc = XCreateGC(g.dpy, w, 0, NULL);
    XPutImage(g.dpy, w, gc, img, 0, 0, 0, 0, (unsigned)width, (unsigned)height);
    XFreeGC(g.dpy, gc);
    XDestroyImage(img); /* frees data too */
    if (behind) {
        XDestroyImage(behind);
    }
    XFlush(g.dpy);
}

static void paint_icon(void)
{
    if (g.icon && g.rgba) {
        put_pixels(g.icon, &g.icon_surface, g.rgba, g.rgba_size, g.rgba_size);
    }
}

/* The tray says which visual it wants; 32-bit ones allow a really transparent background. */
static struct surface tray_surface(Window owner)
{
    struct surface s = {DefaultVisual(g.dpy, g.screen), DefaultDepth(g.dpy, g.screen), false};
    Atom prop = XInternAtom(g.dpy, "_NET_SYSTEM_TRAY_VISUAL", False), type;
    int format;
    unsigned long count, after;
    unsigned char *data = NULL;
    if (XGetWindowProperty(g.dpy, owner, prop, 0, 1, False, XA_VISUALID, &type, &format, &count, &after, &data) ==
            Success && data && count == 1) {
        XVisualInfo want = {.visualid = (VisualID)*(unsigned long *)data}, *found;
        int n = 0;
        found = XGetVisualInfo(g.dpy, VisualIDMask, &want, &n);
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

static void undock(void)
{
    if (g.icon) {
        XDestroyWindow(g.dpy, g.icon);
        g.icon = 0;
    }
    if (g.icon_colormap) {
        XFreeColormap(g.dpy, g.icon_colormap);
        g.icon_colormap = 0;
    }
    g.docked = false;
}

static void dock(Window owner)
{
    undock();
    g.owner = owner;
    if (!owner) {
        return;
    }
    g.icon_surface = tray_surface(owner);
    Window root = RootWindow(g.dpy, g.screen);
    XSetWindowAttributes a;
    memset(&a, 0, sizeof(a));
    unsigned long mask = CWEventMask | CWColormap | CWBorderPixel;
    g.icon_colormap = XCreateColormap(g.dpy, root, g.icon_surface.visual, AllocNone);
    a.colormap = g.icon_colormap;
    a.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask | EnterWindowMask | LeaveWindowMask;
    if (g.icon_surface.has_alpha) {
        mask |= CWBackPixel;
    } else {
        mask |= CWBackPixmap;
        a.background_pixmap = ParentRelative; /* the panel shows through */
    }
    g.size = g.size ? g.size : 22;
    g.icon = XCreateWindow(g.dpy, root, 0, 0, (unsigned)g.size, (unsigned)g.size, 0, g.icon_surface.depth, InputOutput,
                           g.icon_surface.visual, mask, &a);
    XStoreName(g.dpy, g.icon, "Wye");
    XClassHint hint = {(char *)"wye", (char *)"Wye"};
    XSetClassHint(g.dpy, g.icon, &hint);
    unsigned long info[2] = {0, XEMBED_MAPPED};
    Atom xembed = XInternAtom(g.dpy, "_XEMBED_INFO", False);
    XChangeProperty(g.dpy, g.icon, xembed, xembed, 32, PropModeReplace, (unsigned char *)info, 2);

    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = owner;
    ev.xclient.message_type = XInternAtom(g.dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = OPCODE_REQUEST_DOCK;
    ev.xclient.data.l[2] = (long)g.icon;
    XSendEvent(g.dpy, owner, False, NoEventMask, &ev);
    XMapRaised(g.dpy, g.icon); /* XEmbed: the panel only shows a docked icon once it is mapped */
    XFlush(g.dpy);
    g.docked = true;
}

/* The tray can appear after us, or restart: keep looking. */
static void check_owner(uint64_t now)
{
    if (now < g.next_check) {
        return;
    }
    g.next_check = now + RECHECK_MS;
    Window owner = XGetSelectionOwner(g.dpy, g.selection);
    if (owner != g.owner) {
        dock(owner);
    }
}

static int row_at(int y)
{
    int top = 0;
    for (int i = 0; i < g.item_count; ++i) {
        int h = g.items[i].label[0] ? PLAT_TRAY_ROW_H : PLAT_TRAY_SEPARATOR_H;
        if (y >= top && y < top + h) {
            return g.items[i].label[0] && g.items[i].enabled ? i : -1;
        }
        top += h;
    }
    return -1;
}

static void close_popup(void)
{
    if (g.popup) {
        if (g.menu_open) {
            XUngrabPointer(g.dpy, CurrentTime);
        }
        XDestroyWindow(g.dpy, g.popup);
        XFlush(g.dpy);
        g.popup = 0;
    }
    g.menu_open = g.tip_open = false;
}

static const struct surface *screen_surface(void)
{
    static struct surface s;
    s = (struct surface){DefaultVisual(g.dpy, g.screen), DefaultDepth(g.dpy, g.screen), false};
    return &s;
}

static void paint_popup(const struct plat_tray_item *items, int count, int hover)
{
    int w = 0, h = 0;
    uint8_t *rgba = g.painter ? g.painter(items, count, hover, &w, &h) : NULL;
    if (rgba) {
        if (g.popup && w == g.popup_w && h == g.popup_h) {
            put_pixels(g.popup, screen_surface(), rgba, w, h);
        }
        free(rgba);
    }
}

/* Opens next to the icon, inside the screen, on whichever side has room. */
static void open_popup(const struct plat_tray_item *items, int count)
{
    int w = 0, h = 0, x = 0, y = 0;
    Window root = RootWindow(g.dpy, g.screen), child;
    uint8_t *rgba = g.painter ? g.painter(items, count, -1, &w, &h) : NULL;
    if (!rgba) {
        return;
    }
    XTranslateCoordinates(g.dpy, g.icon, root, 0, 0, &x, &y, &child);
    int sw = DisplayWidth(g.dpy, g.screen), sh = DisplayHeight(g.dpy, g.screen);
    x = x + w > sw ? sw - w : x;
    y = y + g.size + h > sh ? y - h : y + g.size;
    y = y < 0 ? 0 : y;
    XSetWindowAttributes a;
    memset(&a, 0, sizeof(a));
    a.override_redirect = True;
    a.save_under = True;
    a.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | LeaveWindowMask;
    g.popup = XCreateWindow(g.dpy, root, x, y, (unsigned)w, (unsigned)h, 0, CopyFromParent, InputOutput, CopyFromParent,
                            CWOverrideRedirect | CWSaveUnder | CWEventMask, &a);
    g.popup_w = w;
    g.popup_h = h;
    XMapRaised(g.dpy, g.popup);
    XSync(g.dpy, False);
    put_pixels(g.popup, screen_surface(), rgba, w, h);
    free(rgba);
}

static void open_menu(void)
{
    close_popup();
    if (!g.item_count) {
        return;
    }
    open_popup(g.items, g.item_count);
    if (!g.popup) {
        return;
    }
    g.menu_open = true;
    g.hover_row = -1;
    g.menu_opened = plat_now_ms();
    /* A click anywhere else closes it. */
    XGrabPointer(g.dpy, g.popup, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync,
                 GrabModeAsync, None, None, CurrentTime);
    XFlush(g.dpy);
}

static void open_tip(void)
{
    if (!g.tooltip[0] || g.popup) {
        return;
    }
    memcpy(g.tip.label, g.tooltip, sizeof(g.tip.label));
    g.tip.enabled = true;
    open_popup(&g.tip, 1);
    g.tip_open = g.popup != 0;
}

static void on_icon_event(const XEvent *ev)
{
    switch (ev->type) {
    case Expose:
        if (ev->xexpose.count == 0) {
            paint_icon();
        }
        break;
    case ConfigureNotify:
        if (ev->xconfigure.width >= 8 && ev->xconfigure.width != g.size) {
            g.size = ev->xconfigure.width; /* the UI gives us a new picture at this size */
        }
        break;
    case DestroyNotify:
        g.icon = 0;
        g.docked = false;
        g.owner = 0;
        break;
    case EnterNotify:
        g.hovering = true;
        g.tip_at = plat_now_ms() + TIP_DELAY_MS;
        break;
    case LeaveNotify:
        g.hovering = false;
        if (g.tip_open) {
            close_popup();
        }
        break;
    case ButtonPress:
        g.hovering = false;
        close_popup();
        if (ev->xbutton.button == BUTTON_LEFT) {
            push(PLAT_TRAY_ACTIVATE);
        } else if (ev->xbutton.button == BUTTON_RIGHT) {
            open_menu();
        }
        break;
    default:
        break;
    }
}

static void on_popup_event(const XEvent *ev)
{
    if (!g.menu_open) {
        if (g.tip_open && ev->type == Expose && ev->xexpose.count == 0) {
            paint_popup(&g.tip, 1, -1);
        }
        return;
    }
    if (ev->type == Expose && ev->xexpose.count == 0) {
        paint_popup(g.items, g.item_count, g.hover_row);
    } else if (ev->type == MotionNotify) {
        bool inside = ev->xmotion.x >= 0 && ev->xmotion.x < g.popup_w;
        int row = inside ? row_at(ev->xmotion.y) : -1;
        if (row != g.hover_row) {
            g.hover_row = row;
            paint_popup(g.items, g.item_count, row);
        }
    } else if (ev->type == ButtonPress) {
        const XButtonEvent *b = &ev->xbutton;
        if (b->x < 0 || b->y < 0 || b->x >= g.popup_w || b->y >= g.popup_h) {
            close_popup();
        }
    } else if (ev->type == ButtonRelease && plat_now_ms() - g.menu_opened >= OPEN_GUARD_MS) {
        const XButtonEvent *b = &ev->xbutton;
        int row = b->x >= 0 && b->x < g.popup_w ? row_at(b->y) : -1;
        if (row >= 0) {
            push(g.items[row].id);
            close_popup();
        }
    }
}

bool plat_tray_start(plat_tray_painter painter)
{
    if (g.dpy) {
        return true;
    }
    g.dpy = XOpenDisplay(NULL);
    if (!g.dpy) {
        return false;
    }
    g.prev_errors = XSetErrorHandler(on_x_error);
    g.painter = painter;
    g.screen = DefaultScreen(g.dpy);
    char name[40];
    snprintf(name, sizeof(name), "_NET_SYSTEM_TRAY_S%d", g.screen);
    g.selection = XInternAtom(g.dpy, name, False);
    g.next_check = 0;
    g.owner = 0;
    g.size = 22;
    return true;
}

void plat_tray_stop(void)
{
    if (!g.dpy) {
        return;
    }
    close_popup();
    undock();
    XSync(g.dpy, False);
    XSetErrorHandler(g.prev_errors);
    XCloseDisplay(g.dpy);
    free(g.rgba);
    memset(&g, 0, sizeof(g));
}

bool plat_tray_available(void)
{
    return g.dpy && g.docked;
}

int plat_tray_icon_size(void)
{
    return g.size ? g.size : 22;
}

void plat_tray_set_icon(const uint8_t *rgba, int size)
{
    uint8_t *copy = malloc((size_t)size * (size_t)size * 4);
    if (!g.dpy || !copy) {
        free(copy);
        return;
    }
    memcpy(copy, rgba, (size_t)size * (size_t)size * 4);
    free(g.rgba);
    g.rgba = copy;
    g.rgba_size = size;
    paint_icon();
}

void plat_tray_set_tooltip(const char *text)
{
    snprintf(g.tooltip, sizeof(g.tooltip), "%s", text);
}

void plat_tray_set_menu(const struct plat_tray_item *items, int count)
{
    g.item_count = count > PLAT_TRAY_ITEMS ? PLAT_TRAY_ITEMS : count;
    memcpy(g.items, items, (size_t)g.item_count * sizeof(items[0]));
    if (g.menu_open) {
        close_popup(); /* it changed under the pointer: better gone than wrong */
    }
}

int plat_tray_poll(void)
{
    if (!g.dpy) {
        return PLAT_TRAY_NONE;
    }
    uint64_t now = plat_now_ms();
    check_owner(now);
    while (XPending(g.dpy) > 0) {
        XEvent ev;
        XNextEvent(g.dpy, &ev);
        if (g.icon && ev.xany.window == g.icon) {
            on_icon_event(&ev);
        } else if (g.popup && ev.xany.window == g.popup) {
            on_popup_event(&ev);
        }
    }
    if (g.hovering && !g.popup && g.tip_at && now >= g.tip_at) {
        g.tip_at = 0;
        open_tip();
    }
    if (!g.queued) {
        return PLAT_TRAY_NONE;
    }
    int id = g.queue[0];
    memmove(g.queue, g.queue + 1, (size_t)--g.queued * sizeof(g.queue[0]));
    return id;
}
