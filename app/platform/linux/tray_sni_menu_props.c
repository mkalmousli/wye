/* The menu's own D-Bus properties (Version, TextDirection, Status) that a panel reads. */
#include "tray_sni.h"

#include <string.h>

static const struct {
    const char *name, *sig;
} MENU_PROPS[] = {{"Version", "u"}, {"TextDirection", "s"}, {"Status", "s"}};

static bool menu_write(dbus_iter *v, const char *name)
{
    if (!strcmp(name, "Version")) {
        dbus_put_u32(v, 3);
    } else if (!strcmp(name, "TextDirection")) {
        dbus_put_str(v, "ltr");
    } else {
        dbus_put_str(v, "normal");
    }
    return true;
}

static bool menu_props_get(dbus_iter *reply, const char *name)
{
    for (size_t i = 0; i < sizeof(MENU_PROPS) / sizeof(MENU_PROPS[0]); i++) {
        if (!strcmp(MENU_PROPS[i].name, name)) {
            return dbus_put_variant(reply, MENU_PROPS[i].sig, menu_write, name);
        }
    }
    return false;
}

static void menu_props_all(dbus_iter *reply)
{
    dbus_iter arr;
    dbus.open_container(reply, 'a', "{sv}", &arr);
    for (size_t i = 0; i < sizeof(MENU_PROPS) / sizeof(MENU_PROPS[0]); i++) {
        dbus_put_entry(&arr, MENU_PROPS[i].name, MENU_PROPS[i].sig, menu_write, MENU_PROPS[i].name);
    }
    dbus.close_container(reply, &arr);
}

bool sni_menu_props(dbus_iter *out, DBusMessage *m, const char *member)
{
    const char *name = NULL;
    if (!strcmp(member, "Get") && dbus_arg(m, 1, 's', &name)) {
        return menu_props_get(out, name);
    }
    if (!strcmp(member, "GetAll")) {
        menu_props_all(out);
        return true;
    }
    return false;
}
