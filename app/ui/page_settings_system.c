/* Settings: updates, startup and device discovery. */
#include "ui.h"
#include "ui_theme.h"

#include "plat_sys.h"
#include "product.h"
#include "raygui.h"

static const char *update_text(const struct wye_update_status *u)
{
    switch (u->state) {
    case WYE_UPD_CHECKING: return "Checking for updates...";
    case WYE_UPD_CURRENT: return "You have the latest version.";
    case WYE_UPD_AVAILABLE: return TextFormat("Version %s is available.", u->manifest.version);
    case WYE_UPD_DOWNLOADING: return "Downloading and verifying update...";
    case WYE_UPD_RESTARTING: return "Restarting to finish the update...";
    case WYE_UPD_ERROR: return TextFormat("Update check failed: %s", u->error);
    default: return "Not checked yet.";
    }
}

static void updates_row(struct wye_ui *ui, Rectangle c, float y)
{
    ui_text(ui, "Software updates", c.x, y + 9, UI_FONT, UI_TEXT);
    int mode = ui->cfg.update_mode;
    mode = ui_segmented((Rectangle){c.x + 200, y, 330, UI_ROW}, "Off;Notify;Automatic", mode);
    if (mode != ui->cfg.update_mode) {
        ui->cfg.update_mode = mode;
        ui_save_config(ui);
    }
    if (ui_button((Rectangle){c.x + 552, y, 130, UI_ROW}, "Check now")) {
        wye_updater_check_now();
    }
    if (ui->upd.state == WYE_UPD_AVAILABLE && ui_button_primary((Rectangle){c.x + 690, y, 150, UI_ROW}, "Install")) {
        wye_updater_install_now();
    }
    ui_text(ui, update_text(&ui->upd), c.x + 200, y + UI_ROW + 6, UI_FONT_SMALL, UI_MUTED);
}

void settings_system_card(struct wye_ui *ui, Rectangle r)
{
    Rectangle c = ui_card(ui, r, "System");
    updates_row(ui, c, c.y);
    float y = c.y + UI_ROW + 34;
    if (ui_toggle((Rectangle){c.x, y, 300, UI_ROW}, "Update device firmware automatically", &ui->cfg.auto_firmware)) {
        ui_save_config(ui);
    }
    bool login = ui->cfg.start_at_login;
    if (ui_toggle((Rectangle){c.x + 400, y, 300, UI_ROW}, "Start with my computer", &login)) {
        if (plat_autostart_set(login)) {
            ui->cfg.start_at_login = login;
            ui_save_config(ui);
        } else {
            ui_toast(ui, "Could not change the startup setting");
        }
    }
    y += UI_ROW + 4;
    if (ui_toggle((Rectangle){c.x, y, 300, UI_ROW}, "Look for Picos with old firmware", &ui->cfg.probe_generic_pico)) {
        ui_save_config(ui);
    }
    int open_as = ui->cfg.start_hidden ? 2 : ui->cfg.start_mini ? 1 : 0, before = open_as;
    ui_text(ui, "Open as", c.x + 400, y + 9, UI_FONT_SMALL, UI_MUTED);
    open_as = ui_segmented((Rectangle){c.x + 462, y, 276, UI_ROW}, "Window;Mini;Hidden", open_as);
    if (open_as != before) {
        ui->cfg.start_hidden = open_as == 2;
        ui->cfg.start_mini = open_as == 1;
        ui_save_config(ui);
    }
}
