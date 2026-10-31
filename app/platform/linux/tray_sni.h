/* State and internal pieces shared by the StatusNotifierItem tray backend (tray_sni*.c). */
#ifndef WYE_TRAY_SNI_H
#define WYE_TRAY_SNI_H

#include "dbus_lib.h"
#include "plat_tray.h"
#include "tray_backends.h"

#include <stdbool.h>
#include <stdint.h>

#define SNI_ITEM_PATH "/StatusNotifierItem"
#define SNI_ITEM_IFACE "org.kde.StatusNotifierItem"
#define SNI_MENU_PATH "/MenuBar"
#define SNI_MENU_IFACE "com.canonical.dbusmenu"
#define SNI_PROPS_IFACE "org.freedesktop.DBus.Properties"

struct sni_state {
    DBusConnection *conn;  /* shared with the process, so never closed; NULL while the backend is off */
    bool registered;       /* the panel's watcher has taken the item */
    char service[64];      /* our well-known bus name */
    uint8_t *rgba;
    int rgba_size;
    char tooltip[PLAT_TRAY_LABEL];
    struct plat_tray_item items[PLAT_TRAY_ITEMS];
    int item_count;
    uint32_t menu_rev;
    int queue[PLAT_TRAY_ITEMS], queued; /* chosen ids, for sni_poll */
};
extern struct sni_state g_sni;

void sni_push(int id);                                      /* tray_sni.c */
void sni_emit(const char *signal);                          /* tray_sni.c: a signal from the item */
bool sni_item_connect(DBusConnection *c);                   /* tray_sni_item.c: name, filter, first registration */

bool sni_register(void);                                    /* tray_sni_watch.c: take the item in with the watcher */
void sni_watch_match(DBusConnection *c);                    /* tray_sni_watch.c */
void sni_watch_changed(DBusMessage *m);                     /* tray_sni_watch.c: the watcher came or went */

bool sni_props_get(dbus_iter *reply, const char *name);     /* tray_sni_props.c */
bool sni_props_all(dbus_iter *reply);
void sni_put_pixmaps(dbus_iter *it, bool with_icon);        /* tray_sni_pixmap.c */
void sni_put_tooltip(dbus_iter *v);                         /* tray_sni_pixmap.c */

void sni_menu_call(DBusConnection *c, DBusMessage *m);      /* tray_sni_menu.c */
void sni_menu_changed(void);                                /* tray_sni_menu.c */
void sni_menu_layout(dbus_iter *out);                       /* tray_sni_layout.c */
bool sni_menu_props(dbus_iter *out, DBusMessage *m, const char *member); /* tray_sni_menu_props.c */

#endif
