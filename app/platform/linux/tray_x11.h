/* The X11 system tray's state and the pieces shared by the tray_x11*.c files. */
#ifndef WYE_TRAY_X11_H
#define WYE_TRAY_X11_H

#include "plat_tray.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>
#include <stdint.h>

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

struct x11_state {
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
};
extern struct x11_state x11;

/* tray_x11_events.c */
int x11_on_x_error(Display *dpy, XErrorEvent *ev);
void x11_push(int id);
void x11_pump(uint64_t now);
int x11_take_choice(void);

/* tray_x11_pixels.c */
void x11_put_pixels(Window w, const struct surface *s, const uint8_t *rgba, int width, int height);

/* tray_x11_visual.c */
struct surface x11_tray_surface(Window owner);
const struct surface *x11_screen_surface(void);
void x11_paint_icon(void);
void x11_paint_popup(const struct plat_tray_item *items, int count, int hover);

/* tray_x11_dock.c */
void x11_undock(void);
void x11_dock(Window owner);
void x11_check_owner(uint64_t now);

/* tray_x11_popup.c */
int x11_row_at(int y);
void x11_close_popup(void);
void x11_open_popup(const struct plat_tray_item *items, int count);
void x11_open_tip(void);

/* tray_x11_menu.c */
void x11_open_menu(void);
void x11_on_popup_event(const XEvent *ev);

#endif
