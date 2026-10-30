/* One function per page; each draws into the content area right of the sidebar. */
#ifndef WYE_UI_PAGES_H
#define WYE_UI_PAGES_H

#include "raylib.h"

struct wye_ui;

/* Pages */
void page_home(struct wye_ui *ui, Rectangle area);
void page_routines(struct wye_ui *ui, Rectangle area);
void page_device(struct wye_ui *ui, Rectangle area);
void page_settings(struct wye_ui *ui, Rectangle area);
void page_about(struct wye_ui *ui, Rectangle area);
void page_setup(struct wye_ui *ui, Rectangle area);
void page_devices(struct wye_ui *ui, Rectangle area);
void identity_panel(struct wye_ui *ui, Rectangle area);  /* USB identity editor body */
void page_identity(struct wye_ui *ui, Rectangle area);
/* Mini mode (ui_mini.c, ui_mini_panel.c) */
void ui_mini_enter(struct wye_ui *ui);
void ui_mini_leave(struct wye_ui *ui);
void ui_mini_panel(struct wye_ui *ui);
void ui_mini_follow_control(struct wye_ui *ui);

#endif
