/* The menu's layout for the panel: a root with one child per item (child ids are the position plus one). */
#include "tray_sni.h"

static void put_item_props(dbus_iter *pr, const struct plat_tray_item *item)
{
    const char *label = item->label;
    int32_t enabled = item->enabled, on = 1;
    if (!label[0]) {
        const char *type = "separator";
        dbus_put_kv(pr, "type", 's', &type);
        return;
    }
    dbus_put_kv(pr, "label", 's', &label);
    dbus_put_kv(pr, "enabled", 'b', &enabled);
    if (item->checked) { /* only checked items get a tick, as on the X11 tray */
        const char *kind = "checkmark";
        dbus_put_kv(pr, "toggle-type", 's', &kind);
        dbus_put_kv(pr, "toggle-state", 'i', &on);
    }
}

void sni_menu_layout(dbus_iter *out)
{
    dbus_iter root, props, kids, var, st, pr, none;
    dbus_put_u32(out, g_sni.menu_rev);
    dbus.open_container(out, 'r', NULL, &root);
    dbus_put_i32(&root, 0);
    dbus.open_container(&root, 'a', "{sv}", &props);
    dbus.close_container(&root, &props);
    dbus.open_container(&root, 'a', "v", &kids);
    for (int i = 0; i < g_sni.item_count; i++) {
        dbus.open_container(&kids, 'v', "(ia{sv}av)", &var);
        dbus.open_container(&var, 'r', NULL, &st);
        dbus_put_i32(&st, i + 1);
        dbus.open_container(&st, 'a', "{sv}", &pr);
        put_item_props(&pr, &g_sni.items[i]);
        dbus.close_container(&st, &pr);
        dbus.open_container(&st, 'a', "v", &none);
        dbus.close_container(&st, &none);
        dbus.close_container(&var, &st);
        dbus.close_container(&kids, &var);
    }
    dbus.close_container(&root, &kids);
    dbus.close_container(out, &root);
}
