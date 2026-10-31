/* The StatusNotifierItem's icon and tooltip values, in the shape a panel reads them. */
#include "tray_sni.h"

/* Panels want ARGB32 pixels in network byte order: A, R, G, B. */
void sni_put_pixmaps(dbus_iter *it, bool with_icon)
{
    dbus_iter arr, px, bytes;
    dbus.open_container(it, 'a', "(iiay)", &arr);
    if (with_icon && g_sni.rgba) {
        int n = g_sni.rgba_size;
        dbus.open_container(&arr, 'r', NULL, &px);
        dbus_put_i32(&px, n);
        dbus_put_i32(&px, n);
        dbus.open_container(&px, 'a', "y", &bytes);
        for (size_t i = 0; i < (size_t)n * (size_t)n; i++) {
            const uint8_t *p = g_sni.rgba + i * 4;
            uint8_t argb[4] = {p[3], p[0], p[1], p[2]};
            for (int k = 0; k < 4; k++) {
                dbus.append_basic(&bytes, 'y', &argb[k]);
            }
        }
        dbus.close_container(&px, &bytes);
        dbus.close_container(&arr, &px);
    }
    dbus.close_container(it, &arr);
}

void sni_put_tooltip(dbus_iter *v)
{
    dbus_iter st;
    dbus.open_container(v, 'r', NULL, &st);
    dbus_put_str(&st, "");          /* icon name */
    sni_put_pixmaps(&st, false);
    dbus_put_str(&st, "Wye");       /* title */
    dbus_put_str(&st, g_sni.tooltip[0] ? g_sni.tooltip : "Wye");
    dbus.close_container(v, &st);
}
