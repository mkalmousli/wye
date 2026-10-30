/* Routines: recording, renaming and deleting. */
#include "routines.h"
#include "ui_theme.h"

#include "plat_sys.h"
#include "raygui.h"
#include "util/util.h"

#include <string.h>

static double g_confirm_delete_until;

/* A finished recording: tidied up if the user wants that, and selected. */
static void after_recording(struct wye_ui *ui)
{
    char path[PLAT_PATH_MAX], err[96];
    if (!wye_engine_last_recording(path, sizeof(path))) {
        return;
    }
    if (ui->cfg.routine_compress) {
        struct wye_routine_doc d = {0};
        struct wye_routine_compress o = {true, WYE_ROUTINE_MERGE_MS, true, WYE_ROUTINE_MAX_PAUSE_MS};
        if (wye_routine_doc_load(&d, path, err, sizeof(err))) {
            size_t removed = wye_routine_doc_compress(&d, &o);
            if (wye_routine_doc_save(&d, path)) {
                ui_toast(ui, TextFormat("Saved, without %lu useless events", (unsigned long)removed));
            }
        }
        wye_routine_doc_free(&d);
    }
    routines_edit_close();
    routines_rescan(path);
}

static void manage_row(struct wye_ui *ui, Rectangle c, float y, const struct wye_routine_info *sel)
{
    float w = (c.width - 8) / 2;
    if (!sel || routines_edit_dirty()) {
        GuiDisable();
    }
    if (ui_button((Rectangle){c.x, y, w, UI_ROW}, ui->routine_rename ? "Save name" : "Rename")) {
        if (ui->routine_rename) {
            char err[128];
            if (!wye_routine_rename(sel->path, ui->routine_name, err, sizeof(err))) {
                ui_toast(ui, err);
            }
            ui->routine_name[0] = '\0';
            routines_rescan(NULL);
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
            routines_edit_close();
            routines_rescan(NULL);
        } else {
            g_confirm_delete_until = GetTime() + 3.0;
        }
    }
    GuiEnable();
    if (ui_button((Rectangle){c.x, y + UI_ROW + 6, c.width, UI_ROW}, "Open folder")) {
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
    Rectangle box = {c.x, c.y, c.width - 88, UI_ROW};
    if (GuiTextBox(box, ui->routine_name, sizeof(ui->routine_name), ui->routine_name_edit)) {
        ui->routine_name_edit = !ui->routine_name_edit;
    }
    if (!ui->routine_name[0] && !ui->routine_name_edit) {
        ui_text(ui, "Name (optional)", box.x + 10, box.y + 9, UI_FONT_SMALL, UI_MUTED);
    }
    Rectangle btn = {c.x + c.width - 80, c.y, 80, UI_ROW};
    if (s->recording) {
        if (ui_button_primary(btn, "Stop")) {
            wye_engine_record(false, NULL);
            ui->routine_name[0] = '\0';
            after_recording(ui);
        }
        ui_dot(c.x + 8, c.y + UI_ROW + 22, UI_BAD);
        ui_text(ui, "Recording. Watch it on the right.", c.x + 22, c.y + UI_ROW + 13, UI_FONT_SMALL, UI_TEXT);
    } else if (!ui->routine_rename) {
        if (s->playing || routines_edit_dirty()) {
            GuiDisable();
        }
        if (ui_button_primary(btn, "Record") && !wye_engine_record(true, ui->routine_name)) {
            ui_toast(ui, "Could not start recording");
        }
        GuiEnable();
        ui_text(ui, "Records what you send to the target.", c.x, c.y + UI_ROW + 13, UI_FONT_SMALL, UI_MUTED);
    }
    manage_row(ui, c, c.y + UI_ROW + 44, sel);
    bool tidy = ui->cfg.routine_compress;
    if (ui_toggle((Rectangle){c.x, c.y + 3 * UI_ROW + 56, c.width, 30}, "Compress new recordings", &tidy)) {
        ui->cfg.routine_compress = tidy;
        ui_save_config(ui);
    }
}
