/*
 * The system tray icon and its menu. Everything runs on the UI thread: call
 * plat_tray_poll every frame. On a desktop with no tray nothing shows, and
 * plat_tray_available says so. A panel that restarts gets the icon back by itself.
 */
#ifndef WYE_PLAT_TRAY_H
#define WYE_PLAT_TRAY_H

#include <stdbool.h>
#include <stdint.h>

#define PLAT_TRAY_LABEL 64
#define PLAT_TRAY_ITEMS 16
#define PLAT_TRAY_NONE (-1)
#define PLAT_TRAY_ACTIVATE (-2)  /* the icon itself was clicked */
#define PLAT_TRAY_ROW_H 28
#define PLAT_TRAY_SEPARATOR_H 9

struct plat_tray_item {
    char label[PLAT_TRAY_LABEL]; /* empty: a separator */
    int id;
    bool enabled, checked;
};

/* Paints menu rows into a new RGBA buffer (freed with free()), for systems whose tray has no menu
   of its own; hover is a row index or -1. Row heights are the PLAT_TRAY_* above. */
typedef uint8_t *(*plat_tray_painter)(const struct plat_tray_item *items, int count, int hover, int *w, int *h);

bool plat_tray_start(plat_tray_painter painter);
void plat_tray_stop(void);
bool plat_tray_available(void);         /* the desktop has a tray and the icon is in it */
int plat_tray_icon_size(void);          /* in pixels: what plat_tray_set_icon should be given */
void plat_tray_set_icon(const uint8_t *rgba, int size);
void plat_tray_set_tooltip(const char *text);
void plat_tray_set_menu(const struct plat_tray_item *items, int count);
/* The id of a chosen menu item, PLAT_TRAY_ACTIVATE, or PLAT_TRAY_NONE. */
int plat_tray_poll(void);

#endif
