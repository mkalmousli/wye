/* Devices: keyboard and mouse - how the pointer feels, the jiggler, and what the keys do. */
#include "devices.h"
#include "ui_fx.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include "raygui.h"

static void heading(struct wye_ui *ui, Rectangle c, enum ui_icon icon, const char *title, Color tint)
{
    DrawCircleV((Vector2){c.x + 16, c.y + 14}, 16, Fade(tint, 0.18f));
    ui_icon(icon, (Vector2){c.x + 16, c.y + 14}, tint, false);
    ui_text_bold(ui, title, c.x + 42, c.y + 3, 20, UI_TEXT);
}

static bool slider(struct wye_ui *ui, Rectangle r, const char *label, const char *value, float *v, float lo, float hi)
{
    float before = *v;
    ui_text(ui, label, r.x, r.y, UI_FONT_SMALL, UI_MUTED);
    ui_text_bold(ui, value, r.x + r.width - ui_measure(value, UI_FONT_SMALL, true).x, r.y, UI_FONT_SMALL,
                 UI_TEXT);
    ui_slider((Rectangle){r.x, r.y + 24, r.width, 14}, v, lo, hi);
    return *v != before;
}

static void mouse(struct wye_ui *ui, Rectangle c)
{
    heading(ui, c, UI_ICON_JIGGLE, "Mouse", UI_ACCENT2);
    float sens = (float)ui->cfg.sensitivity, jig = (float)ui->cfg.jiggler_interval_s, y = c.y + 52;
    bool changed = slider(ui, (Rectangle){c.x, y, c.width, 40}, "Pointer speed on the other computer",
                          TextFormat("%d%%", ui->cfg.sensitivity), &sens, 10, 400);
    changed |= slider(ui, (Rectangle){c.x, y + 62, c.width, 40}, "Jiggle every",
                      TextFormat("%d s", ui->cfg.jiggler_interval_s), &jig, 10, 600);
    ui->cfg.sensitivity = (int)sens / 5 * 5;
    ui->cfg.jiggler_interval_s = (int)jig / 10 * 10;
    changed |= ui_toggle((Rectangle){c.x, y + 120, 300, UI_ROW}, "Reverse the scroll wheel", &ui->cfg.invert_wheel);
    if (changed) {
        ui_save_config(ui);
        wye_engine_apply_config(&ui->cfg);
    }
    if (ui_button_tinted((Rectangle){c.x, y + 170, 200, UI_ROW}, ui->st.jiggler_on ? "Stop jiggling" : "Start jiggling",
                         ui->st.jiggler_on ? UI_OK : UI_ACCENT)) {
        wye_engine_jiggler(!ui->st.jiggler_on);
    }
}

static void keyboard(struct wye_ui *ui, Rectangle c)
{
    heading(ui, c, UI_ICON_KEYBOARD, "Keyboard", UI_ACCENT);
    const char *key = ui_key_name(ui->cfg.hotkey);
    ui_text(ui, "Switch computers with", c.x, c.y + 56, UI_FONT_SMALL, UI_MUTED);
    ui_keycap(ui, (Rectangle){c.x, c.y + 80, 70, 46}, key, true, UI_ACCENT);
    ui_text(ui, "Tap it to switch. Hold it with another key for an action", c.x + 86, c.y + 86, UI_FONT_SMALL,
            UI_TEXT);
    ui_text(ui, "(change both on the Shortcuts page).", c.x + 86, c.y + 106, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, "Media keys (volume, play, brightness) reach the other computer too.", c.x, c.y + 150,
            UI_FONT_SMALL, UI_TEXT);
    ui_text(ui, "\"Type clipboard\" types text with a US keyboard layout.", c.x, c.y + 172, UI_FONT_SMALL, UI_MUTED);
}

void devices_input(struct wye_ui *ui, Rectangle r)
{
    float gap = 16, w = (r.width - gap) / 2, h = r.height < 320 ? r.height : 320; /* as tall as the content */
    mouse(ui, ui_card(ui, (Rectangle){r.x, r.y, w, h}, NULL));
    keyboard(ui, ui_card(ui, (Rectangle){r.x + w + gap, r.y, w, h}, NULL));
}
