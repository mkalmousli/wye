/* Routines: list of recordings and playback controls. */
#include "ui.h"
#include "ui_theme.h"

#include "features/routine.h"
#include "raygui.h"

void routines_record_card(struct wye_ui *ui, Rectangle r, const struct wye_routine_info *sel);

static struct wye_routine_info g_list[WYE_ROUTINE_LIST_MAX];
static int g_count;
static double g_scanned;

static void list_card(struct wye_ui *ui, Rectangle r)
{
    Rectangle c = ui_card(ui, r, TextFormat("Saved routines (%d)", g_count));
    if (g_count == 0) {
        ui_text(ui, "Nothing recorded yet. Press Record, then use the target as usual.", c.x, c.y + 8,
                UI_FONT_SMALL, UI_MUTED);
        return;
    }
    int rows = (int)(c.height / 30);
    ui->routine_scroll -= (int)GetMouseWheelMove();
    int max_scroll = g_count > rows ? g_count - rows : 0;
    ui->routine_scroll = ui->routine_scroll < 0 ? 0 : (ui->routine_scroll > max_scroll ? max_scroll : ui->routine_scroll);
    for (int i = 0; i < rows && i + ui->routine_scroll < g_count; ++i) {
        int idx = i + ui->routine_scroll;
        Rectangle row = {c.x - 6, c.y + (float)i * 30, c.width + 12, 28};
        bool hot = CheckCollisionPointRec(GetMousePosition(), row);
        if (idx == ui->routine_sel || hot) {
            DrawRectangleRounded(row, 0.3f, 6, idx == ui->routine_sel ? Fade(UI_ACCENT, 0.25f) : UI_CARD_EDGE);
        }
        ui_text(ui, g_list[idx].name, row.x + 10, row.y + 5, UI_FONT_SMALL, UI_TEXT);
        if (hot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ui->routine_sel = idx;
        }
    }
}

static void playback_card(struct wye_ui *ui, Rectangle r, const struct wye_routine_info *sel)
{
    Rectangle c = ui_card(ui, r, "Playback");
    const struct wye_status *s = &ui->st;
    float w = (c.width - 3 * 8) / 4;
    static bool loop;
    if (!sel || s->recording || s->link.state != WYE_LINK_READY) {
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
    if (ui_button((Rectangle){c.x + w + 8, c.y, w, UI_ROW}, s->play_paused ? "Resume" : "Pause")) {
        wye_engine_play_pause();
    }
    if (ui_button((Rectangle){c.x + 2 * (w + 8), c.y, w, UI_ROW}, "Stop")) {
        wye_engine_play_stop();
    }
    GuiEnable();
    ui_toggle((Rectangle){c.x + 3 * (w + 8), c.y, w, UI_ROW}, "Loop", &loop);
    float y = c.y + UI_ROW + 14;
    if (ui_button((Rectangle){c.x, y, 40, 28}, "-")) {
        wye_engine_play_speed(-25);
    }
    ui_text(ui, TextFormat("Speed %d%%", s->play_speed), c.x + 52, y + 5, UI_FONT_SMALL, UI_TEXT);
    if (ui_button((Rectangle){c.x + 140, y, 40, 28}, "+")) {
        wye_engine_play_speed(25);
    }
    if (s->playing) {
        ui_text(ui, s->play_name, c.x + 200, y + 5, UI_FONT_SMALL, UI_MUTED);
        ui_progress((Rectangle){c.x, y + 40, c.width, 6}, s->play_progress, UI_TARGET);
    }
}

void page_routines(struct wye_ui *ui, Rectangle a)
{
    if (GetTime() - g_scanned > 1.0) {
        g_scanned = GetTime();
        g_count = wye_routine_list(g_list, WYE_ROUTINE_LIST_MAX);
    }
    if (ui->routine_sel >= g_count) {
        ui->routine_sel = g_count - 1;
    }
    const struct wye_routine_info *sel = ui->routine_sel >= 0 && g_count ? &g_list[ui->routine_sel] : NULL;
    ui_text_bold(ui, "Routines", a.x, a.y, UI_FONT_TITLE, UI_TEXT);
    float half = (a.width - UI_PAD) / 2;
    list_card(ui, (Rectangle){a.x, a.y + 44, half, a.height - 44});
    routines_record_card(ui, (Rectangle){a.x + half + UI_PAD, a.y + 44, half, 200}, sel);
    playback_card(ui, (Rectangle){a.x + half + UI_PAD, a.y + 256, half, 170}, sel);
}
