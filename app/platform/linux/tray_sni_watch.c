/* The panel's watcher: Wye takes its item in with the watcher, and again whenever the watcher comes back. */
#include "tray_sni.h"

#include <string.h>

#define WATCHER "org.kde.StatusNotifierWatcher"
#define WATCHER_PATH "/StatusNotifierWatcher"
#define WATCHER_MATCH \
    "type='signal',sender='org.freedesktop.DBus',interface='org.freedesktop.DBus'," \
    "member='NameOwnerChanged',arg0='" WATCHER "'"

bool sni_register(void)
{
    dbus_iter it;
    DBusMessage *m = dbus.call_new(WATCHER, WATCHER_PATH, WATCHER, "RegisterStatusNotifierItem");
    if (!m) {
        return false;
    }
    dbus.iter_init_append(m, &it);
    dbus_put_str(&it, g_sni.service);
    dbus_err e;
    dbus.err_init(&e);
    DBusMessage *r = dbus.send_block(g_sni.conn, m, 1000, &e);
    dbus.unref(m);
    bool ok = r != NULL;
    if (r) {
        dbus.unref(r);
    }
    dbus.err_free(&e);
    return ok;
}

void sni_watch_match(DBusConnection *c)
{
    dbus_err e;
    dbus.err_init(&e);
    dbus.bus_add_match(c, WATCHER_MATCH, &e);
    dbus.err_free(&e);
}

void sni_watch_changed(DBusMessage *m)
{
    const char *member = dbus.member(m), *name = NULL, *owner = NULL;
    if (!member || strcmp(member, "NameOwnerChanged") || !dbus_arg(m, 0, 's', &name) || strcmp(name, WATCHER) ||
        !dbus_arg(m, 2, 's', &owner)) {
        return;
    }
    g_sni.registered = owner[0] && sni_register();
}
