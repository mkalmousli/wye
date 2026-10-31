/* The StatusNotifierItem's menu, as com.canonical.dbusmenu on /MenuBar: the panel draws it natively.
   Menu item ids are the item's position plus one; 0 is the menu root. The layout is tray_sni_layout.c,
   the menu's properties are tray_sni_menu_props.c. */
#include "tray_sni.h"

#include <string.h>

static void on_event(DBusMessage *m)
{
    int32_t id = 0;
    const char *event = NULL;
    if (!dbus_arg(m, 0, 'i', &id) || !dbus_arg(m, 1, 's', &event) || strcmp(event, "clicked")) {
        return;
    }
    if (id >= 1 && id <= g_sni.item_count && g_sni.items[id - 1].label[0]) {
        sni_push(g_sni.items[id - 1].id);
    }
}

void sni_menu_call(DBusConnection *c, DBusMessage *m)
{
    const char *iface = dbus.iface(m), *member = dbus.member(m);
    DBusMessage *r = dbus.return_new(m);
    if (!r || !member) {
        dbus_send_unref(c, r);
        return;
    }
    dbus_iter out;
    dbus.iter_init_append(r, &out);
    if (iface && !strcmp(iface, SNI_PROPS_IFACE)) {
        sni_menu_props(&out, m, member);
    } else if (!strcmp(member, "GetLayout")) {
        sni_menu_layout(&out);
    } else if (!strcmp(member, "Event")) {
        on_event(m);
    } else if (!strcmp(member, "AboutToShow")) {
        dbus_put_bool(&out, false); /* the menu does not change when it opens */
    } else if (!strcmp(member, "GetGroupProperties")) {
        dbus_iter empty;
        dbus.open_container(&out, 'a', "(ia{sv})", &empty);
        dbus.close_container(&out, &empty);
    }
    dbus_send_unref(c, r);
}

void sni_menu_changed(void)
{
    if (!g_sni.conn || !g_sni.registered) {
        return;
    }
    DBusMessage *s = dbus.signal_new(SNI_MENU_PATH, SNI_MENU_IFACE, "LayoutUpdated");
    if (!s) {
        return;
    }
    dbus_iter it;
    dbus.iter_init_append(s, &it);
    dbus_put_u32(&it, g_sni.menu_rev);
    dbus_put_i32(&it, 0);
    dbus_send_unref(g_sni.conn, s);
}
