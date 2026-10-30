/* Settings: what happens on taking control, and system options. */
#include "ui.h"
#include "ui_theme.h"

#include "raygui.h"

void settings_system_card(struct wye_ui *ui, Rectangle r);

static void control_card(struct wye_ui *ui, Rectangle r)
{
    Rectangle c = ui_card(ui, r, "Taking control");
    float y = c.y;
    ui_text(ui, "On taking control", c.x, y + 9, UI_FONT, UI_TEXT);
    int hud = ui->cfg.hud_mode;
    hud = ui_segmented((Rectangle){c.x + 200, y, 320, UI_ROW}, "Keep the window;Go to the mini strip", hud);
    if (hud != ui->cfg.hud_mode) {
        ui->cfg.hud_mode = hud;
        ui_save_config(ui);
    }
    ui_text(ui, "The hotkey is under Shortcuts. Mouse speed, scrolling and the jiggler are under Devices.", c.x, y + UI_ROW + 18, UI_FONT_SMALL,
            UI_MUTED);
}

void page_settings(struct wye_ui *ui, Rectangle a)
{
    ui_text_bold(ui, "Settings", a.x, a.y - 4, 30, UI_TEXT);
    ui_text(ui, "How you switch, and how Wye runs on this computer.", a.x + 136, a.y + 8, UI_FONT_SMALL,
            UI_MUTED);
    control_card(ui, (Rectangle){a.x, a.y + 56, a.width, 136});
    settings_system_card(ui, (Rectangle){a.x, a.y + 208, a.width, a.height - 208 < 316 ? a.height - 208 : 316});
}
