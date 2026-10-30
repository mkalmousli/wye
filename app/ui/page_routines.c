/* Routines: record on the left with the saved list, the event editor and playback on the right. */
#include "routines.h"
#include "ui_theme.h"

#include "raygui.h"
#include "util/util.h"

#include <string.h>

static struct wye_routine_info g_list[WYE_ROUTINE_LIST_MAX];
static int g_count;
static double g_scanned;
static char g_select_path[PLAT_PATH_MAX];

void routines_rescan(const char *select_path)
{
    g_scanned = 0;
    if (select_path) {
        wye_strcpy(g_select_path, sizeof(g_select_path), select_path);
    }
}

static void list_card(struct wye_ui *ui, Rectangle r)
{
    Rectangle c = ui_card(ui, r, TextFormat("Saved routines (%d)", g_count));
    if (g_count == 0) {
        ui_text(ui, "Nothing recorded yet.", c.x, c.y + 8, UI_FONT_SMALL, UI_MUTED);
        return;
    }
    int rows = (int)(c.height / 30);
    if (CheckCollisionPointRec(GetMousePosition(), r)) {
        ui->routine_scroll -= (int)GetMouseWheelMove();
    }
    int max_scroll = g_count > rows ? g_count - rows : 0;
    ui->routine_scroll = ui->routine_scroll < 0 ? 0 : (ui->routine_scroll > max_scroll ? max_scroll : ui->routine_scroll);
    for (int i = 0; i < rows && i + ui->routine_scroll < g_count; ++i) {
        int idx = i + ui->routine_scroll;
        Rectangle row = {c.x - 6, c.y + (float)i * 30, c.width + 12, 28};
        bool hot = CheckCollisionPointRec(GetMousePosition(), row);
        if (idx == ui->routine_sel || hot) {
            DrawRectangleRounded(row, 0.3f, 6, idx == ui->routine_sel ? Fade(UI_ACCENT, 0.25f) : UI_CARD_EDGE);
        }
        BeginScissorMode((int)row.x, (int)row.y, (int)row.width, (int)row.height);
        ui_text(ui, g_list[idx].name, row.x + 10, row.y + 5, UI_FONT_SMALL, UI_TEXT);
        EndScissorMode();
        if (hot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && idx != ui->routine_sel) {
            if (routines_edit_dirty()) {
                ui_toast(ui, "Save or revert your changes first");
            } else {
                ui->routine_sel = idx;
            }
        }
    }
}

static void playback_card(struct wye_ui *ui, Rectangle r, const struct wye_routine_info *sel)
{
    static const int SPEEDS[] = {50, 100, 200, 400, 1000};
    Rectangle c = ui_card(ui, r, NULL);
    const struct wye_status *s = &ui->st;
    float w = 76;
    static bool loop;
    if (!sel || s->recording || s->link.state != WYE_LINK_READY || routines_edit_dirty()) {
        GuiDisable();
    }
    if (ui_button_primary((Rectangle){c.x, c.y, w, UI_ROW}, "Play")) {
        char err[128];
        if (!wye_engine_play(sel->path, loop, err, sizeof(err))) {
            ui_toast(ui, err);
        }
    }
    GuiEnable();
    if (!s->playing) {
        GuiDisable();
    }
    if (ui_button((Rectangle){c.x + w + 6, c.y, w, UI_ROW}, s->play_paused ? "Resume" : "Pause")) {
        wye_engine_play_pause();
    }
    if (ui_button((Rectangle){c.x + 2 * (w + 6), c.y, w, UI_ROW}, "Stop")) {
        wye_engine_play_stop();
    }
    GuiEnable();
    ui_toggle((Rectangle){c.x + 3 * (w + 6), c.y, 80, UI_ROW}, "Loop", &loop);
    int chosen = 1;
    for (int i = 0; i < 5; ++i) {
        chosen = s->play_speed == SPEEDS[i] ? i : chosen;
    }
    float seg_x = c.x + c.width - 190;
    int pick = ui_segmented((Rectangle){seg_x, c.y, 190, UI_ROW}, "x.5;x1;x2;x4;x10", chosen);
    if (pick != chosen) {
        wye_engine_play_speed(SPEEDS[pick]);
    }
    if (s->playing) {
        ui_progress((Rectangle){c.x, c.y + UI_ROW + 10, c.width, 6}, s->play_progress, UI_TARGET);
    }
}

void page_routines(struct wye_ui *ui, Rectangle a)
{
    if (GetTime() - g_scanned > 1.0) {
        g_scanned = GetTime();
        g_count = wye_routine_list(g_list, WYE_ROUTINE_LIST_MAX);
        for (int i = 0; g_select_path[0] && i < g_count; ++i) {
            if (strcmp(g_list[i].path, g_select_path) == 0) {
                ui->routine_sel = i;
                g_select_path[0] = '\0';
                break;
            }
        }
    }
    if (ui->routine_sel >= g_count) {
        ui->routine_sel = g_count - 1;
    }
    const struct wye_routine_info *sel = ui->routine_sel >= 0 && g_count ? &g_list[ui->routine_sel] : NULL;
    routines_edit_open(ui, sel);
    ui_text_bold(ui, "Routines", a.x, a.y - 4, 30, UI_TEXT);
    float left = 280, h = a.height - 48, y = a.y + 48;
    routines_record_card(ui, (Rectangle){a.x, y, left, 256}, sel);
    list_card(ui, (Rectangle){a.x, y + 256 + UI_PAD, left, h - 256 - UI_PAD});
    float x = a.x + left + UI_PAD, w = a.width - left - UI_PAD, play_h = 88;
    routines_edit_card(ui, (Rectangle){x, y, w, h - play_h - UI_PAD}, sel);
    playback_card(ui, (Rectangle){x, y + h - play_h, w, play_h}, sel);
}
