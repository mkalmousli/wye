/* The Linux system tray. StatusNotifierItem over D-Bus when the desktop's panel offers it, otherwise
   the X11 XEmbed tray. Only one backend is live; calls for the other are no-ops. */
#include "plat_tray.h"
#include "tray_backends.h"

static bool via_sni;

bool plat_tray_start(plat_tray_painter painter)
{
    via_sni = sni_start();
    if (via_sni) {
        return true;
    }
    sni_stop();
    return x11_tray_start(painter);
}

void plat_tray_stop(void)
{
    sni_stop();
    x11_tray_stop();
    via_sni = false;
}

bool plat_tray_available(void)
{
    return via_sni ? sni_available() : x11_tray_available();
}

int plat_tray_icon_size(void)
{
    return via_sni ? 22 : x11_tray_icon_size();
}

void plat_tray_set_icon(const uint8_t *rgba, int size)
{
    if (via_sni) {
        sni_set_icon(rgba, size);
    } else {
        x11_tray_set_icon(rgba, size);
    }
}

void plat_tray_set_tooltip(const char *text)
{
    if (via_sni) {
        sni_set_tooltip(text);
    } else {
        x11_tray_set_tooltip(text);
    }
}

void plat_tray_set_menu(const struct plat_tray_item *items, int count)
{
    if (via_sni) {
        sni_set_menu(items, count);
    } else {
        x11_tray_set_menu(items, count);
    }
}

int plat_tray_poll(void)
{
    int id = via_sni ? sni_poll() : PLAT_TRAY_NONE;
    return id != PLAT_TRAY_NONE ? id : x11_tray_poll();
}
