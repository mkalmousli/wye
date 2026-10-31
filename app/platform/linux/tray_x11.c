/*
 * System tray on X11, using only Xlib: the freedesktop "system tray" protocol (XEmbed). KDE and others
 * that moved on still offer it through a bridge; plain GNOME has no tray without an extension.
 * This file holds the entry points; the parts are tray_x11_*.c. It uses its own X connection.
 */
#include "tray_x11.h"
#include "tray_backends.h"

#include "plat_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct x11_state x11;

bool x11_tray_start(plat_tray_painter painter)
{
    if (x11.dpy) {
        return true;
    }
    x11.dpy = XOpenDisplay(NULL);
    if (!x11.dpy) {
        return false;
    }
    x11.prev_errors = XSetErrorHandler(x11_on_x_error);
    x11.painter = painter;
    x11.screen = DefaultScreen(x11.dpy);
    char name[40];
    snprintf(name, sizeof(name), "_NET_SYSTEM_TRAY_S%d", x11.screen);
    x11.selection = XInternAtom(x11.dpy, name, False);
    x11.next_check = 0;
    x11.owner = 0;
    x11.size = 22;
    return true;
}

void x11_tray_stop(void)
{
    if (!x11.dpy) {
        return;
    }
    x11_close_popup();
    x11_undock();
    XSync(x11.dpy, False);
    XSetErrorHandler(x11.prev_errors);
    XCloseDisplay(x11.dpy);
    free(x11.rgba);
    memset(&x11, 0, sizeof(x11));
}

bool x11_tray_available(void)
{
    return x11.dpy && x11.docked;
}

int x11_tray_icon_size(void)
{
    return x11.size ? x11.size : 22;
}

void x11_tray_set_icon(const uint8_t *rgba, int size)
{
    uint8_t *copy = malloc((size_t)size * (size_t)size * 4);
    if (!x11.dpy || !copy) {
        free(copy);
        return;
    }
    memcpy(copy, rgba, (size_t)size * (size_t)size * 4);
    free(x11.rgba);
    x11.rgba = copy;
    x11.rgba_size = size;
    x11_paint_icon();
}

void x11_tray_set_tooltip(const char *text)
{
    snprintf(x11.tooltip, sizeof(x11.tooltip), "%s", text);
}

void x11_tray_set_menu(const struct plat_tray_item *items, int count)
{
    x11.item_count = count > PLAT_TRAY_ITEMS ? PLAT_TRAY_ITEMS : count;
    memcpy(x11.items, items, (size_t)x11.item_count * sizeof(items[0]));
    if (x11.menu_open) {
        x11_close_popup(); /* it changed under the pointer: better gone than wrong */
    }
}

int x11_tray_poll(void)
{
    if (!x11.dpy) {
        return PLAT_TRAY_NONE;
    }
    uint64_t now = plat_now_ms();
    x11_check_owner(now);
    x11_pump(now);
    return x11_take_choice();
}
