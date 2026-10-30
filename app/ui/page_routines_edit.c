/*
 * Routines: the event list. While recording it follows the events live. Otherwise it is an
 * editor for the selected routine; changes stay in memory until saved.
 */
#include "routines.h"
#include "ui_theme.h"

#include "raygui.h"
#include "util/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROW_H 22.0f

static struct {
    struct wye_routine_doc doc;
    char path[PLAT_PATH_MAX];
    bool dirty, hide_moves, merge, pauses;
    long sel_from, sel_to;      /* selected events, -1: none */
    long scroll;
    size_t *shown;              /* doc indexes after the filter */
    size_t shown_count;
    bool shown_stale;
    char delay[12];
    int editing;
    bool defaults_set;
} g = {.sel_from = -1, .sel_to = -1};

bool routines_edit_dirty(void)
{
    return g.dirty;
}

void routines_edit_close(void)
{
    wye_routine_doc_free(&g.doc);
    g.path[0] = '\0';
    g.dirty = false;
    g.sel_from = g.sel_to = -1;
    g.scroll = 0;
    g.shown_stale = true;
}

void routines_edit_open(struct wye_ui *ui, const struct wye_routine_info *sel)
{
    if (!g.defaults_set) {
        g.defaults_set = true;
        g.merge = g.pauses = true;
    }
    if (!sel) {
        routines_edit_close();
        return;
    }
    if (strcmp(g.path, sel->path) == 0 || g.dirty) {
        return; /* unsaved edits stay open, even if the list moved under them */
    }
    char err[96];
    routines_edit_close();
    wye_strcpy(g.path, sizeof(g.path), sel->path);
    if (!wye_routine_doc_load(&g.doc, sel->path, err, sizeof(err))) {
        ui_toast(ui, err);
    }
}

static void changed(void)
{
    g.dirty = true;
    g.shown_stale = true;
}

static void refilter(void)
{
    if (!g.shown_stale) {
        return;
    }
    g.shown_stale = false;
    free(g.shown);
    g.shown = malloc((g.doc.count + 1) * sizeof(*g.shown));
    g.shown_count = 0;
    for (size_t i = 0; g.shown && i < g.doc.count; ++i) {
        if (!g.hide_moves || !wye_routine_is_move(&g.doc.ev[i])) {
            g.shown[g.shown_count++] = i;
        }
    }
}

static void draw_row(struct wye_ui *ui, Rectangle row, const struct wye_routine_event *ev, uint32_t delay, Color tint)
{
    if (tint.a) {
        DrawRectangleRounded(row, 0.3f, 4, tint);
    }
    BeginScissorMode((int)row.x, (int)row.y, (int)row.width, (int)row.height);
    ui_text(ui, TextFormat("%.3f", (double)ev->t / 1000.0), row.x + 8, row.y + 3, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, TextFormat("+%u", delay), row.x + 78, row.y + 3, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, routines_event_text(ev), row.x + 150, row.y + 3, UI_FONT_SMALL, UI_TEXT);
    EndScissorMode();
}

static void header(struct wye_ui *ui, float x, float y)
{
    ui_text(ui, "Time (s)", x + 8, y, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, "Wait (ms)", x + 78, y, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, "Event", x + 150, y, UI_FONT_SMALL, UI_MUTED);
}

static void live_view(struct wye_ui *ui, Rectangle c)
{
    static struct wye_routine_event tail[WYE_ROUTINE_TAIL];
    ui_dot(c.x + 6, c.y + 9, UI_BAD);
    ui_text_bold(ui, TextFormat("Recording - %lu events", ui->st.recorded_events), c.x + 20, c.y, UI_FONT, UI_TEXT);
    ui_toggle((Rectangle){c.x + c.width - 180, c.y - 6, 180, 28}, "Hide mouse moves", &g.hide_moves);
    header(ui, c.x, c.y + 32);
    int rows = (int)((c.height - 56) / ROW_H), n = wye_engine_recording_tail(tail, WYE_ROUTINE_TAIL);
    static int picked[WYE_ROUTINE_TAIL];
    int m = 0;
    for (int i = n - 1; i >= 0 && m < rows; --i) { /* the newest ones that pass the filter */
        if (!g.hide_moves || !wye_routine_is_move(&tail[i])) {
            picked[m++] = i;
        }
    }
    if (!m) {
        ui_text(ui, TextFormat("Take control with %s and use the other computer: it all shows up here.",
                               ui_key_name(ui->cfg.hotkey)), c.x + 8, c.y + 60, UI_FONT_SMALL, UI_MUTED);
    }
    for (int i = 0; i < m; ++i) { /* newest at the bottom */
        const struct wye_routine_event *ev = &tail[picked[m - 1 - i]];
        int at = picked[m - 1 - i];
        uint32_t delay = at > 0 ? ev->t - tail[at - 1].t : 0;
        draw_row(ui, (Rectangle){c.x, c.y + 54 + (float)i * ROW_H, c.width, ROW_H - 1}, ev, delay, BLANK);
    }
}

static void select_row(long idx)
{
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (shift && g.sel_from >= 0) {
        g.sel_to = idx;
    } else {
        g.sel_from = g.sel_to = idx;
        snprintf(g.delay, sizeof(g.delay), "%u", wye_routine_doc_delay(&g.doc, (size_t)idx));
    }
}

static void sel_range(size_t *from, size_t *to)
{
    *from = (size_t)(g.sel_from < g.sel_to ? g.sel_from : g.sel_to);
    *to = (size_t)(g.sel_from < g.sel_to ? g.sel_to : g.sel_from);
}

static void table(struct wye_ui *ui, Rectangle r, const struct wye_routine_info *sel)
{
    refilter();
    header(ui, r.x, r.y);
    ui_toggle((Rectangle){r.x + r.width - 180, r.y - 6, 180, 26}, "Hide mouse moves", &g.hide_moves);
    Rectangle list = {r.x, r.y + 22, r.width, r.height - 22};
    long rows = (long)(list.height / ROW_H), count = (long)g.shown_count;
    bool mine = sel && ui->st.playing && strstr(ui->st.play_name, sel->name) != NULL;
    long playing = -1;
    if (mine) {
        for (long i = 0; i < count; ++i) { /* the last shown event already played */
            if (g.shown[i] + 1 > (size_t)ui->st.play_index) {
                break;
            }
            playing = i;
        }
        if (playing >= 0 && (playing < g.scroll || playing >= g.scroll + rows)) {
            g.scroll = playing; /* follow along */
        }
    }
    if (CheckCollisionPointRec(GetMousePosition(), list)) {
        g.scroll -= (long)GetMouseWheelMove() * 3;
    }
    long max_scroll = count > rows ? count - rows : 0;
    g.scroll = g.scroll < 0 ? 0 : (g.scroll > max_scroll ? max_scroll : g.scroll);
    if (!count) {
        ui_text(ui, g.doc.count ? "Only mouse moves here: they are hidden." : "No events.", r.x + 8, list.y + 8,
                UI_FONT_SMALL, UI_MUTED);
    }
    size_t from = 0, to = 0;
    if (g.sel_from >= 0) {
        sel_range(&from, &to);
    }
    for (long i = 0; i < rows && i + g.scroll < count; ++i) {
        long at = i + g.scroll;
        size_t idx = g.shown[at];
        Rectangle row = {list.x, list.y + (float)i * ROW_H, list.width, ROW_H - 1};
        bool hot = CheckCollisionPointRec(GetMousePosition(), row);
        bool picked = g.sel_from >= 0 && idx >= from && idx <= to;
        Color tint = at == playing ? Fade(UI_TARGET, 0.35f) : picked ? Fade(UI_ACCENT, 0.3f) : hot ? UI_CARD_EDGE : BLANK;
        draw_row(ui, row, &g.doc.ev[idx], wye_routine_doc_delay(&g.doc, idx), tint);
        if (hot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            select_row((long)idx);
        }
    }
}

static void selection_tools(struct wye_ui *ui, Rectangle c, float y)
{
    bool any = g.sel_from >= 0 && !ui->st.playing, single = any && g.sel_from == g.sel_to;
    float w = 76, x = c.x;
    if (!any) {
        GuiDisable(); /* a disabled button never reports a click */
    }
    if (ui_button((Rectangle){x, y, w, 30}, "Delete")) {
        size_t from, to;
        sel_range(&from, &to);
        wye_routine_doc_delete(&g.doc, from, to);
        g.sel_from = g.sel_to = -1;
        changed();
    }
    x += w + 4;
    if (ui_button((Rectangle){x, y, w, 30}, "Copy")) {
        size_t from, to;
        sel_range(&from, &to);
        if (wye_routine_doc_duplicate(&g.doc, from, to)) {
            changed();
        }
    }
    GuiEnable();
    x += w + 4;
    if (!single) {
        GuiDisable();
    }
    for (int dir = -1; dir <= 1; dir += 2) {
        if (ui_button((Rectangle){x, y, 34, 30}, dir < 0 ? "^" : "v") && single &&
            wye_routine_doc_move(&g.doc, (size_t)g.sel_from, dir)) {
            g.sel_from += dir;
            g.sel_to = g.sel_from;
            changed();
        }
        x += 38;
    }
    x += 6;
    ui_text(ui, "Wait", x, y + 7, UI_FONT_SMALL, UI_MUTED);
    x += 36;
    ui_textbox(ui, (Rectangle){x, y, 70, 30}, NULL, g.delay, sizeof(g.delay), 1, &g.editing);
    x += 74;
    if (ui_button((Rectangle){x, y, 44, 30}, "Set") && single) {
        wye_routine_doc_set_delay(&g.doc, (size_t)g.sel_from, (uint32_t)strtoul(g.delay, NULL, 10));
        changed();
    }
    GuiEnable();
}

static void tidy_tools(struct wye_ui *ui, Rectangle c, float y)
{
    bool usable = g.doc.count && !ui->st.playing;
    if (!usable) {
        GuiDisable();
    }
    if (ui_button((Rectangle){c.x, y, 96, 30}, "Compress")) {
        struct wye_routine_compress o = {g.merge, WYE_ROUTINE_MERGE_MS, g.pauses, WYE_ROUTINE_MAX_PAUSE_MS};
        size_t removed = wye_routine_doc_compress(&g.doc, &o);
        g.sel_from = g.sel_to = -1;
        changed();
        ui_toast(ui, TextFormat("Removed %lu events. Save to keep this.", (unsigned long)removed));
    }
    GuiEnable();
    ui_toggle((Rectangle){c.x + 104, y, 130, 30}, "Merge moves", &g.merge);
    ui_toggle((Rectangle){c.x + 252, y, 150, 30}, "Shorten pauses", &g.pauses);
    if (!usable || ui->st.play_speed == 100) {
        GuiDisable();
    }
    /* Playback speed is only for playing; this writes it into the routine itself. */
    const char *bake = TextFormat("Apply x%g", (double)ui->st.play_speed / 100.0);
    if (ui_button((Rectangle){c.x + c.width - 110, y, 110, 30}, bake)) {
        wye_routine_doc_speed(&g.doc, ui->st.play_speed);
        wye_engine_play_speed(100);
        changed();
        ui_toast(ui, "The events are now that much faster, and playback is back at x1. Save to keep this.");
    }
    GuiEnable();
}

static void save_tools(struct wye_ui *ui, Rectangle c, float y)
{
    if (!g.dirty || ui->st.playing) {
        GuiDisable();
    }
    if (ui_button_primary((Rectangle){c.x, y, 96, 30}, "Save")) {
        if (wye_routine_doc_save(&g.doc, g.path)) {
            g.dirty = false;
            ui_toast(ui, "Saved");
        } else {
            ui_toast(ui, "Could not save this routine");
        }
    }
    if (ui_button((Rectangle){c.x + 104, y, 96, 30}, "Revert")) {
        char path[PLAT_PATH_MAX], err[96];
        wye_strcpy(path, sizeof(path), g.path);
        routines_edit_close();
        wye_strcpy(g.path, sizeof(g.path), path);
        if (!wye_routine_doc_load(&g.doc, path, err, sizeof(err))) {
            ui_toast(ui, err);
        }
    }
    GuiEnable();
    uint32_t total = g.doc.count ? g.doc.ev[g.doc.count - 1].t : 0;
    ui_text(ui, TextFormat("%lu events, %.1f s%s", (unsigned long)g.doc.count, (double)total / 1000.0,
                           g.dirty ? " - not saved" : ""), c.x + 212, y + 7, UI_FONT_SMALL, g.dirty ? UI_WARN : UI_MUTED);
}

void routines_edit_card(struct wye_ui *ui, Rectangle r, const struct wye_routine_info *sel)
{
    Rectangle c = ui_card(ui, r, NULL);
    if (ui->st.recording) {
        live_view(ui, c);
        return;
    }
    if (!sel) {
        ui_text(ui, "Pick a routine, or record a new one.", c.x, c.y, UI_FONT_SMALL, UI_MUTED);
        return;
    }
    static bool last_hide;
    if (last_hide != g.hide_moves) {
        last_hide = g.hide_moves;
        g.shown_stale = true;
    }
    float tools = 3 * 36;
    table(ui, (Rectangle){c.x, c.y, c.width, c.height - tools}, sel);
    float y = c.y + c.height - tools + 4;
    selection_tools(ui, c, y);
    tidy_tools(ui, c, y + 36);
    save_tools(ui, c, y + 72);
}
