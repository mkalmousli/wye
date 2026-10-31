/* What the app sets on the StatusNotifierItem: icon, tooltip and menu. Each change tells the panel. */
#include "tray_sni.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sni_set_icon(const uint8_t *rgba, int size)
{
    size_t bytes = (size_t)size * (size_t)size * 4;
    uint8_t *copy = malloc(bytes);
    if (!copy) {
        return;
    }
    memcpy(copy, rgba, bytes);
    free(g_sni.rgba);
    g_sni.rgba = copy;
    g_sni.rgba_size = size;
    sni_emit("NewIcon");
}

void sni_set_tooltip(const char *text)
{
    snprintf(g_sni.tooltip, sizeof(g_sni.tooltip), "%s", text);
    sni_emit("NewToolTip");
}

void sni_set_menu(const struct plat_tray_item *items, int count)
{
    g_sni.item_count = count > PLAT_TRAY_ITEMS ? PLAT_TRAY_ITEMS : count;
    memcpy(g_sni.items, items, (size_t)g_sni.item_count * sizeof(items[0]));
    g_sni.menu_rev++;
    sni_menu_changed();
}
