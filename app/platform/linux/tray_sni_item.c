/* The StatusNotifierItem object: answers the panel's property reads and its left click, and joins the
   panel's watcher (tray_sni_watch.c). The watcher may come and go, so Wye registers again when it returns. */
#include "tray_sni.h"

#include <string.h>

static bool wired; /* the filter and the match rule are added once per process */

static void props_call(dbus_iter *out, DBusMessage *m, const char *member)
{
    const char *name = NULL;
    if (!strcmp(member, "Get") && dbus_arg(m, 1, 's', &name)) {
        sni_props_get(out, name);
    } else if (!strcmp(member, "GetAll")) {
        sni_props_all(out);
    }
}

static void item_call(DBusConnection *c, DBusMessage *m)
{
    const char *iface = dbus.iface(m), *member = dbus.member(m);
    DBusMessage *r = dbus.return_new(m);
    if (!r) {
        return;
    }
    dbus_iter out;
    dbus.iter_init_append(r, &out);
    if (iface && member && !strcmp(iface, SNI_PROPS_IFACE)) {
        props_call(&out, m, member);
    } else if (member && !strcmp(member, "Activate")) { /* left click on the icon */
        sni_push(PLAT_TRAY_ACTIVATE);
    }
    dbus_send_unref(c, r);
}

static int on_message(DBusConnection *c, DBusMessage *m, void *user)
{
    (void)user;
    const char *path = dbus.path(m);
    int type = dbus.msg_type(m);
    if (type == WDBUS_METHOD_CALL && path && !strcmp(path, SNI_ITEM_PATH)) {
        item_call(c, m);
        return WDBUS_HANDLED;
    }
    if (type == WDBUS_METHOD_CALL && path && !strcmp(path, SNI_MENU_PATH)) {
        sni_menu_call(c, m);
        return WDBUS_HANDLED;
    }
    if (type == WDBUS_SIGNAL) {
        sni_watch_changed(m);
    }
    return WDBUS_NOT_HANDLED;
}

bool sni_item_connect(DBusConnection *c)
{
    dbus_err e;
    if (!wired) {
        wired = true;
        dbus.add_filter(c, on_message, NULL, NULL);
        sni_watch_match(c);
    }
    dbus.err_init(&e);
    int r = dbus.bus_request_name(c, g_sni.service, WDBUS_NAME_DO_NOT_QUEUE, &e);
    dbus.err_free(&e);
    if (r != 1 && r != 4) { /* 1: we took the name; 4: we already had it */
        return false;
    }
    g_sni.conn = c;
    g_sni.registered = sni_register();
    return g_sni.registered;
}
