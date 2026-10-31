/* The StatusNotifierItem's properties: what a panel reads to draw Wye's icon, tooltip and menu link. */
#include "tray_sni.h"

#include <string.h>

static const struct {
    const char *name, *sig;
} PROPS[] = {
    {"Category", "s"},           {"Id", "s"},                       {"Title", "s"},
    {"Status", "s"},             {"WindowId", "i"},                 {"IconName", "s"},
    {"IconPixmap", "a(iiay)"},   {"OverlayIconName", "s"},          {"OverlayIconPixmap", "a(iiay)"},
    {"AttentionIconName", "s"},  {"AttentionIconPixmap", "a(iiay)"}, {"AttentionMovieName", "s"},
    {"ToolTip", "(sa(iiay)ss)"}, {"ItemIsMenu", "b"},               {"Menu", "o"},
};

static const char *text_for(const char *name)
{
    if (!strcmp(name, "Category")) {
        return "ApplicationStatus";
    }
    if (!strcmp(name, "Id")) {
        return "wye";
    }
    if (!strcmp(name, "Title")) {
        return "Wye";
    }
    if (!strcmp(name, "Status")) {
        return "Active";
    }
    return "";
}

static bool write_value(dbus_iter *v, const char *name)
{
    if (!strcmp(name, "WindowId")) {
        dbus_put_i32(v, 0);
    } else if (!strcmp(name, "ItemIsMenu")) {
        dbus_put_bool(v, false);
    } else if (!strcmp(name, "Menu")) {
        dbus_put_path(v, SNI_MENU_PATH);
    } else if (!strcmp(name, "ToolTip")) {
        sni_put_tooltip(v);
    } else if (strstr(name, "Pixmap")) {
        sni_put_pixmaps(v, !strcmp(name, "IconPixmap"));
    } else {
        dbus_put_str(v, text_for(name));
    }
    return true;
}

static const char *sig_of(const char *name)
{
    for (size_t i = 0; i < sizeof(PROPS) / sizeof(PROPS[0]); i++) {
        if (!strcmp(PROPS[i].name, name)) {
            return PROPS[i].sig;
        }
    }
    return NULL;
}

bool sni_props_get(dbus_iter *reply, const char *name)
{
    const char *sig = sig_of(name);
    return sig && dbus_put_variant(reply, sig, write_value, name);
}

bool sni_props_all(dbus_iter *reply)
{
    dbus_iter arr;
    if (!dbus.open_container(reply, 'a', "{sv}", &arr)) {
        return false;
    }
    for (size_t i = 0; i < sizeof(PROPS) / sizeof(PROPS[0]); i++) {
        dbus_put_entry(&arr, PROPS[i].name, PROPS[i].sig, write_value, PROPS[i].name);
    }
    return dbus.close_container(reply, &arr);
}
