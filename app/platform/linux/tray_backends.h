/* The two Linux tray backends behind plat_tray.h: StatusNotifierItem (tray_sni*.c) and XEmbed (tray_x11.c). */
#ifndef WYE_TRAY_BACKENDS_H
#define WYE_TRAY_BACKENDS_H

#include "plat_tray.h"

bool sni_start(void);
void sni_stop(void);
bool sni_available(void);
void sni_set_icon(const uint8_t *rgba, int size);
void sni_set_tooltip(const char *text);
void sni_set_menu(const struct plat_tray_item *items, int count);
int sni_poll(void);

bool x11_tray_start(plat_tray_painter painter);
void x11_tray_stop(void);
bool x11_tray_available(void);
int x11_tray_icon_size(void);
void x11_tray_set_icon(const uint8_t *rgba, int size);
void x11_tray_set_tooltip(const char *text);
void x11_tray_set_menu(const struct plat_tray_item *items, int count);
int x11_tray_poll(void);

#endif
