/*
 * Desktop UI (raylib + raygui). One window with a sidebar of pages; while
 * forwarding it can collapse into a small always-on-top HUD.
 */
#ifndef WYE_UI_H
#define WYE_UI_H

#include "engine/engine.h"
#include "raylib.h"
#include "update/update.h"

enum wye_page {
    PAGE_HOME = 0,
    PAGE_ROUTINES,
    PAGE_DEVICES,    /* what the other computer sees: keyboard, mouse, drive... */
    PAGE_DEVICE,
    PAGE_SHORTCUTS,
    PAGE_SETTINGS,
    PAGE_ABOUT,
    PAGE_SETUP,      /* guided setup wizard (not in the sidebar) */
    PAGE_IDENTITY,   /* USB identity editor, opened from Device */
    PAGE_COUNT,
};

#define UI_TOASTS 4
struct wye_ui {
    struct wye_config cfg;
    struct wye_status st;
    struct wye_update_status upd;
    enum wye_page page;
    Font font, bold, display;      /* display: bold rasterised large for headlines and numbers */
    bool quit, hidden;
    bool mini, mini_dragging;        /* small floating panel instead of the full window */
    Vector2 mini_pos, mini_win0;     /* window position when a drag started */
    int mini_cur0[2], mini_frames;   /* screen pointer at drag start; frames since entering */
    double mini_last_click;
    struct { char text[WYE_NOTICE_LEN]; double until; } toasts[UI_TOASTS];
    int toast_count;
    /* page state */
    int routine_sel, routine_scroll;
    char routine_name[64];
    bool routine_name_edit, routine_rename;
    int setup_step;
    int devices_tab;                 /* which tab of the Devices page is open */
};

int wye_ui_run(struct wye_ui *ui);

/* Shared helpers (ui_common.c, ui_widgets.c, ui_theme.c) */
void ui_theme_load(struct wye_ui *ui);
void ui_theme_unload(struct wye_ui *ui);
void ui_toast(struct wye_ui *ui, const char *text);
void ui_draw_toasts(struct wye_ui *ui);
void ui_save_config(struct wye_ui *ui);
void ui_open_shared_folder(struct wye_ui *ui);
/* The name and colour stored on a Pico, else these defaults (device_pico_name.c). */
const char *ui_pico_name(const struct wye_ui *ui, int node, const char *fallback);
Color ui_pico_color(const struct wye_ui *ui, int node, Color fallback);
/* Text (ui_fonts.c): each size rasterised exactly; sizes below 13 are drawn at 13. */
Font ui_font(float size, bool bold);
float ui_font_size(float size);
Vector2 ui_measure(const char *s, float size, bool bold);
void ui_draw_text(const char *s, float x, float y, float size, bool bold, Color c);
void ui_fonts_unload(void);
void ui_text(struct wye_ui *ui, const char *s, float x, float y, float size, Color c);
void ui_text_bold(struct wye_ui *ui, const char *s, float x, float y, float size, Color c);
Rectangle ui_card(struct wye_ui *ui, Rectangle r, const char *title);
void ui_dot(float x, float y, Color c);
float ui_chip(struct wye_ui *ui, float x, float y, const char *text, Color col); /* a pill label; returns its width */
float ui_chip_width(struct wye_ui *ui, const char *text);
void ui_progress(Rectangle r, int percent, Color c);
bool ui_button(Rectangle r, const char *label);
bool ui_button_primary(Rectangle r, const char *label);
bool ui_button_tinted(Rectangle r, const char *label, Color tint);
bool ui_toggle(Rectangle r, const char *label, bool *value);
int ui_segmented(Rectangle r, const char *items, int selected);   /* items "A;B;C"; returns the choice */
bool ui_slider(Rectangle r, float *value, float lo, float hi);    /* true while it changes */
bool ui_textbox(struct wye_ui *ui, Rectangle r, const char *label, char *buf, int size, int id, int *editing);
const char *ui_key_name(int usage);
const char *ui_status_text(const struct wye_ui *ui, Color *color); /* "Ready"... */
int ui_tabs(struct wye_ui *ui, Rectangle r, const char *const *labels, int count, int selected);

#include "ui_pages.h"

#endif
