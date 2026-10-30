/* Devices: the shared folder's size. */
#include "devices.h"
#include "ui_theme.h"

#include "raygui.h"

#include <stdio.h>
#include <stdlib.h>

static const int SIZES_MB[] = {64, 256, 1024, 8192};

/* Presets, or any size up to 64 GB in whole GB (the image file only grows as files are added). */
void devices_size_row(struct wye_ui *ui, Rectangle c, float y)
{
    static char custom[12];
    int pick = 4;
    for (int i = 0; i < 4; ++i) {
        pick = ui->cfg.drive_size_mb == SIZES_MB[i] ? i : pick;
    }
    int before = pick;
    ui_text(ui, "Size", c.x, y + 9, UI_FONT_SMALL, UI_MUTED);
    pick = ui_segmented((Rectangle){c.x + 48, y, 380, UI_ROW}, "64 MB;256 MB;1 GB;8 GB;Custom", pick);
    if (pick != before && pick < 4) {
        ui->cfg.drive_size_mb = SIZES_MB[pick];
        ui_save_config(ui);
        wye_engine_apply_config(&ui->cfg);
    }
    if (pick == 4) {
        if (*devices_editing() != 5) {
            snprintf(custom, sizeof(custom), "%d", (ui->cfg.drive_size_mb + 1023) / 1024);
        }
        Rectangle box = {c.x + 48 + 5 * 78 + 10, y, 70, UI_ROW};
        if (ui_textbox(ui, box, NULL, custom, sizeof(custom), 5, devices_editing())) {
            int gb = atoi(custom);
            ui->cfg.drive_size_mb = (gb < 1 ? 1 : gb > 64 ? 64 : gb) * 1024;
            ui_save_config(ui);
            wye_engine_apply_config(&ui->cfg);
        }
        ui_text(ui, "GB (up to 64)", box.x + box.width + 8, y + 9, UI_FONT_SMALL, UI_MUTED);
    }
}
