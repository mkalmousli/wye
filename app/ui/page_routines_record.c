/* Routines: recording, renaming and deleting. */
#include "ui.h"
#include "ui_theme.h"

#include "features/routine.h"
#include "plat_sys.h"
#include "raygui.h"
#include "util/util.h"

#include <string.h>

static double g_confirm_delete_until;

static void manage_row(struct wye_ui *ui, Rectangle c, float y, const struct wye_routine_info *sel)
{
    float w = (c.width - 2 * 8) / 3;
    if (!sel) {
        GuiDisable();
    }
    if (ui_button((Rectangle){c.x, y, w, UI_ROW}, ui->routine_rename ? "Save name" : "Rename")) {
        if (ui->routine_rename) {
            char err[128];
            if (!wye_routine_rename(sel->path, ui->routine_name, err, sizeof(err))) {
                ui_toast(ui, err);
            }
            ui->routine_name[0] = '\0';
        } else {
            wye_strcpy(ui->routine_name, sizeof(ui->routine_name), sel->name);
        }
        ui->routine_rename = !ui->routine_rename;
    }
    bool confirming = GetTime() < g_confirm_delete_until;
    if (ui_button((Rectangle){c.x + w + 8, y, w, UI_ROW}, confirming ? "Click to confirm" : "Delete")) {
        if (confirming) {
            wye_routine_delete(sel->path);
            g_confirm_delete_until = 0;
        } else {
            g_confirm_delete_until = GetTime() + 3.0;
        }
    }
    GuiEnable();
    if (ui_button((Rectangle){c.x + 2 * (w + 8), y, w, UI_ROW}, "Open folder")) {
        char dir[PLAT_PATH_MAX];
        if (wye_routines_dir(dir, sizeof(dir))) {
            plat_reveal_path(dir);
        }
    }
}

void routines_record_card(struct wye_ui *ui, Rectangle r, const struct wye_routine_info *sel)
{
    Rectangle c = ui_card(ui, r, ui->routine_rename ? "Rename routine" : "Record");
    const struct wye_status *s = &ui->st;
    Rectangle box = {c.x, c.y, c.width - 128, UI_ROW};
    if (GuiTextBox(box, ui->routine_name, sizeof(ui->routine_name), ui->routine_name_edit)) {
        ui->routine_name_edit = !ui->routine_name_edit;
    }
    if (!ui->routine_name[0] && !ui->routine_name_edit) {
        ui_text(ui, "Name (optional)", box.x + 10, box.y + 9, UI_FONT_SMALL, UI_MUTED);
    }
    Rectangle btn = {c.x + c.width - 120, c.y, 120, UI_ROW};
    if (s->recording) {
        if (ui_button_primary(btn, "Stop")) {
            wye_engine_record(false, NULL);
            ui->routine_name[0] = '\0';
        }
        ui_dot(c.x + 8, c.y + UI_ROW + 22, UI_BAD);
        ui_text(ui, TextFormat("Recording - %lu events", s->recorded_events), c.x + 22, c.y + UI_ROW + 13,
                UI_FONT_SMALL, UI_TEXT);
    } else if (!ui->routine_rename) {
        if (s->playing) {
            GuiDisable();
        }
        if (ui_button_primary(btn, "Record") && !wye_engine_record(true, ui->routine_name)) {
            ui_toast(ui, "Could not start recording");
        }
        GuiEnable();
        ui_text(ui, "Everything sent to the target is recorded.", c.x, c.y + UI_ROW + 13, UI_FONT_SMALL, UI_MUTED);
    }
    manage_row(ui, c, c.y + UI_ROW + 48, sel);
}
