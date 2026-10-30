/* Devices: the shared folder - on/off, where it lives, its name over there, size and status. */
#include "devices.h"
#include "ui_theme.h"

#include "raygui.h"

#include <time.h>

/* On/off is two things: serving the folder (here) and the drive interface (on the Pico). */
void devices_set_drive(struct wye_ui *ui, bool on)
{
    struct wye_functions *f = devices_functions();
    ui->cfg.drive_enabled = on;
    ui_save_config(ui);
    if (f && ((f->flags & WYE_FN_DRIVE) != 0) != on) {
        f->flags = (uint8_t)(on ? f->flags | WYE_FN_DRIVE : f->flags & ~WYE_FN_DRIVE);
        devices_functions_save(ui); /* the drive appears / disappears after the Pico restarts */
    }
}

static void switch_row(struct wye_ui *ui, Rectangle c)
{
    bool on = ui->cfg.drive_enabled;
    if (ui_toggle((Rectangle){c.x, c.y, 360, UI_ROW}, "Share a folder with the other computer", &on)) {
        devices_set_drive(ui, on);
    }
}

static void folder_row(struct wye_ui *ui, Rectangle c, float y)
{
    static char path[WYE_CFG_STR];
    if (*devices_editing() != 2) {
        snprintf(path, sizeof(path), "%.500s", ui->cfg.drive_folder[0] ? ui->cfg.drive_folder : ui->st.drive.folder);
    }
    if (ui_textbox(ui, (Rectangle){c.x, y, c.width - 110, UI_ROW}, "Folder on this computer", path, sizeof(path), 2,
                   devices_editing())) {
        snprintf(ui->cfg.drive_folder, sizeof(ui->cfg.drive_folder), "%s", path);
        ui_save_config(ui);
        wye_engine_apply_config(&ui->cfg);
    }
    if (ui_button((Rectangle){c.x + c.width - 100, y, 100, UI_ROW}, "Open")) {
        plat_reveal_path(ui->st.drive.folder);
    }
}

static void name_row(struct wye_ui *ui, Rectangle c, float y, struct wye_functions *f)
{
    ui_textbox(ui, (Rectangle){c.x, y, 120, UI_ROW}, "Maker", f->drive_vendor, sizeof(f->drive_vendor), 3,
               devices_editing());
    ui_textbox(ui, (Rectangle){c.x + 130, y, 200, UI_ROW}, "Drive name (other computer)", f->drive_product,
               sizeof(f->drive_product), 4, devices_editing());
    if (!devices_functions_dirty(ui)) {
        GuiDisable();
    }
    if (ui_button_primary((Rectangle){c.x + 340, y, 170, UI_ROW}, "Save to the Pico")) {
        devices_functions_save(ui);
    }
    GuiEnable();
}

void devices_folder(struct wye_ui *ui, Rectangle c)
{
    const struct wye_drive_status *d = &ui->st.drive;
    struct wye_functions *f = devices_functions();
    switch_row(ui, c);
    Color col = !d->enabled ? UI_MUTED : d->error ? UI_BAD : d->state == WYE_DRIVE_SYNCED ? UI_OK : UI_ACCENT;
    ui_dot(c.x + 8, c.y + 52, col);
    long ago = d->synced_at ? (long)(time(NULL) - d->synced_at) : -1;
    const char *when = ago < 0 ? "" : ago < 5 ? "  - just now" : ago < 120 ? TextFormat("  - %lds ago", ago)
                                                                           : TextFormat("  - %ld min ago", ago / 60);
    ui_text(ui, d->enabled ? TextFormat("%s%s", d->message, when) : "Off", c.x + 22, c.y + 44, UI_FONT, UI_TEXT);
    if (d->enabled && ui_button((Rectangle){c.x + c.width - 120, c.y + 38, 120, 30}, "Sync now")) {
        wye_engine_drive_sync_now();
    }
    folder_row(ui, c, c.y + 104);
    if (f) {
        name_row(ui, c, c.y + 168, f);
    }
    devices_size_row(ui, c, c.y + 214);
    ui_text(ui, "Both computers work in the same folder: it is a USB drive there and \"Wye Drive\" in your file",
            c.x, c.y + 266, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, "manager here. Changes cross over in about a second (the drive there reconnects for a moment).",
            c.x, c.y + 288, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, "If both edit one file, the other version is kept as \"name (other computer)\". Best for documents.",
            c.x, c.y + 310, UI_FONT_SMALL, UI_MUTED);
}
