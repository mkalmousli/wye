/* The StatusNotifierItem tray backend: Wye's icon in panels that speak the KDE/freedesktop tray protocol
   (KDE, XFCE, Cinnamon, GNOME with the AppIndicator extension). Panels call Wye over D-Bus. The setters
   for icon, tooltip and menu are in tray_sni_set.c. */
#include "tray_sni.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct sni_state g_sni;

bool sni_start(void)
{
    DBusConnection *c = g_sni.conn;
    if (!c) {
        dbus_err e;
        if (!dbus_load()) {
            return false;
        }
        dbus.err_init(&e);
        c = dbus.bus_get(WDBUS_SESSION, &e);
        bool ok = c && !dbus.err_set(&e);
        dbus.err_free(&e);
        if (!ok) {
            return false;
        }
        snprintf(g_sni.service, sizeof(g_sni.service), "org.kde.StatusNotifierItem-%d-1", (int)getpid());
    }
    return sni_item_connect(c);
}

void sni_stop(void)
{
    if (g_sni.conn) {
        dbus_err e;
        dbus.err_init(&e);
        dbus.bus_release_name(g_sni.conn, g_sni.service, &e); /* the panel drops the icon when the name goes */
        dbus.err_free(&e);
        dbus.flush(g_sni.conn);
    }
    g_sni.conn = NULL;
    g_sni.registered = false;
    free(g_sni.rgba);
    g_sni.rgba = NULL;
    g_sni.rgba_size = 0;
    g_sni.queued = 0;
}

bool sni_available(void)
{
    return g_sni.registered;
}

void sni_push(int id)
{
    if (g_sni.queued < PLAT_TRAY_ITEMS) {
        g_sni.queue[g_sni.queued++] = id;
    }
}

void sni_emit(const char *signal)
{
    if (g_sni.conn && g_sni.registered) {
        dbus_send_unref(g_sni.conn, dbus.signal_new(SNI_ITEM_PATH, SNI_ITEM_IFACE, signal));
    }
}

int sni_poll(void)
{
    if (!g_sni.conn) {
        return PLAT_TRAY_NONE;
    }
    if (!dbus.read_write_dispatch(g_sni.conn, 0)) { /* the session bus went away */
        sni_stop();
        return PLAT_TRAY_NONE;
    }
    if (!g_sni.queued) {
        return PLAT_TRAY_NONE;
    }
    int id = g_sni.queue[0];
    memmove(g_sni.queue, g_sni.queue + 1, (size_t)--g_sni.queued * sizeof(g_sni.queue[0]));
    return id;
}
