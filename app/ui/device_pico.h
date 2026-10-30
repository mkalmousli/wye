/* Picos page: one Pico card (device_pico.c). */
#ifndef WYE_UI_DEVICE_PICO_H
#define WYE_UI_DEVICE_PICO_H

#include "ui.h"

struct device_pico {
    const char *title, *where, *missing;  /* title: the role, like "Bridge Pico" */
    const struct wye_node_info *info;
    uint8_t role, node;
    bool outdated;
    Color tint;
};

/* Personalizing (device_pico_name.c). */
bool device_pico_editing(int node);
void device_pico_edit_open(const struct wye_ui *ui, const struct device_pico *p);
void device_pico_editor(struct wye_ui *ui, Rectangle c, const struct device_pico *p);

void device_pico_card(struct wye_ui *ui, Rectangle r, const struct device_pico *p);
void device_stats(struct wye_ui *ui, Rectangle r);   /* latency, errors, uptimes (device_stats.c) */

#endif
