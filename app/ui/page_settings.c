/* Settings: the hotkey, what happens while controlling, and system options. */
#include "ui.h"
#include "ui_theme.h"

#include "raygui.h"

void settings_system_card(struct wye_ui *ui, Rectangle r);

static void hotkey_row(struct wye_ui *ui, Rectangle c, float y)
{
    ui_text(ui, "Switch hotkey", c.x, y + 9, UI_FONT, UI_TEXT);
    if (ui->capturing_hotkey && !ui->st.learning_key && ui->st.learned_key) {
        ui->cfg.hotkey = ui->st.learned_key;
        ui->capturing_hotkey = false;
        ui_save_config(ui);
        ui_toast(ui, TextFormat("Hotkey is now %s", ui_key_name(ui->cfg.hotkey)));
    }
    const char *label = ui->capturing_hotkey ? "Press a key..." : ui_key_name(ui->cfg.hotkey);
    if (ui_button((Rectangle){c.x + 200, y, 180, UI_ROW}, label)) {
        ui->capturing_hotkey = !ui->capturing_hotkey;
        wye_engine_learn_key(ui->capturing_hotkey);
    }
    ui_text(ui, "Tap to switch; hold with V/J/Del/Esc for shortcuts.", c.x + 396, y + 10,
            UI_FONT_SMALL, UI_MUTED);
}

static void control_card(struct wye_ui *ui, Rectangle r)
{
    Rectangle c = ui_card(ui, r, "Taking control");
    hotkey_row(ui, c, c.y);
    float y = c.y + UI_ROW + 14;
    ui_text(ui, "While controlling", c.x, y + 9, UI_FONT, UI_TEXT);
    int hud = ui->cfg.hud_mode;
    hud = ui_segmented((Rectangle){c.x + 200, y, 320, UI_ROW}, "Keep the window;Mini strip", hud);
    if (hud != ui->cfg.hud_mode) {
        ui->cfg.hud_mode = hud;
        ui_save_config(ui);
    }
    ui_text(ui, "Mouse speed, scrolling and the jiggler are under Devices.", c.x, y + UI_ROW + 18, UI_FONT_SMALL,
            UI_MUTED);
}

void page_settings(struct wye_ui *ui, Rectangle a)
{
    ui_text_bold(ui, "Settings", a.x, a.y - 4, 30, UI_TEXT);
    ui_text(ui, "How you switch, and how Wye runs on this computer.", a.x + 136, a.y + 8, UI_FONT_SMALL,
            UI_MUTED);
    control_card(ui, (Rectangle){a.x, a.y + 56, a.width, 186});
    settings_system_card(ui, (Rectangle){a.x, a.y + 258, a.width, a.height - 258 < 236 ? a.height - 258 : 236});
}
