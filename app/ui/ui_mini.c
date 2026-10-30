/* Mini mode: the window becomes a small always-on-top strip on every workspace. */
#include "ui.h"

#include "plat_window.h"

#define MINI_W 324
#define MINI_H 40
#define FULL_MIN_W 860
#define FULL_MIN_H 580

static Vector2 g_full_pos, g_full_size = {1000, 660};

/* Which window it was and where the panel sat, for the next start. */
static void remember(struct wye_ui *ui)
{
    ui->cfg.last_mini = ui->mini;
    ui->cfg.mini_x = (int)ui->mini_pos.x;
    ui->cfg.mini_y = (int)ui->mini_pos.y;
    wye_config_save(&ui->cfg);
}

/* The last place the panel sat, unless that screen has gone. */
void ui_mini_restore_position(struct wye_ui *ui)
{
    for (int m = 0; m < GetMonitorCount(); ++m) {
        Vector2 at = GetMonitorPosition(m);
        if (ui->cfg.mini_x >= at.x && ui->cfg.mini_x + MINI_W <= at.x + GetMonitorWidth(m) &&
            ui->cfg.mini_y >= at.y && ui->cfg.mini_y + MINI_H <= at.y + GetMonitorHeight(m)) {
            ui->mini_pos = (Vector2){(float)ui->cfg.mini_x, (float)ui->cfg.mini_y};
            return;
        }
    }
}

void ui_mini_enter(struct wye_ui *ui)
{
    if (ui->mini) {
        return;
    }
    g_full_pos = GetWindowPosition();
    g_full_size = (Vector2){(float)GetScreenWidth(), (float)GetScreenHeight()};
    SetWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
    ClearWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(MINI_W, MINI_H);
    SetWindowSize(MINI_W, MINI_H);
    if (ui->mini_pos.x == 0 && ui->mini_pos.y == 0) { /* first time: top-right corner */
        int mon = GetCurrentMonitor();
        Vector2 at = GetMonitorPosition(mon);
        ui->mini_pos = (Vector2){at.x + (float)(GetMonitorWidth(mon) - MINI_W - 24), at.y + 40};
    }
    SetWindowPosition((int)ui->mini_pos.x, (int)ui->mini_pos.y);
    ui->mini = true;
    ui->mini_frames = 0;
    remember(ui);
}

void ui_mini_leave(struct wye_ui *ui)
{
    if (!ui->mini) {
        return;
    }
    ui->mini_pos = GetWindowPosition();
    ui->mini_dragging = false;
    ClearWindowState(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(FULL_MIN_W, FULL_MIN_H);
    SetWindowSize((int)g_full_size.x, (int)g_full_size.y);
    SetWindowPosition((int)g_full_pos.x, (int)g_full_pos.y);
    plat_window_to_current_desktop(GetWindowHandle()); /* not back on workspace 1 */
    ui->mini = false;
    remember(ui);
}

/* Window managers may drop "sticky" while they remap the undecorated window: say it a few times. */
void ui_mini_keep_on_all_desktops(struct wye_ui *ui)
{
    if (ui->mini_frames <= 60 && ui->mini_frames % 15 == 0) {
        plat_window_all_desktops(GetWindowHandle(), true);
    }
    ui->mini_frames++;
}

/*
 * Dragging by `grip`, measured with the screen pointer: the window-relative
 * mouse moves with the window itself, which made the old drag jitter.
 * A double click opens the full window.
 */
void ui_mini_drag(struct wye_ui *ui, Rectangle grip)
{
    int cx, cy;
    bool screen = plat_cursor_screen_pos(GetWindowHandle(), &cx, &cy);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), grip)) {
        if (GetTime() - ui->mini_last_click < 0.35) {
            ui_mini_leave(ui);
            return;
        }
        ui->mini_last_click = GetTime();
        ui->mini_dragging = screen;
        ui->mini_win0 = GetWindowPosition();
        ui->mini_cur0[0] = cx;
        ui->mini_cur0[1] = cy;
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (ui->mini_dragging) {
            ui->mini_pos = GetWindowPosition();
            remember(ui);
        }
        ui->mini_dragging = false;
    }
    if (ui->mini_dragging && screen) {
        SetWindowPosition((int)ui->mini_win0.x + cx - ui->mini_cur0[0], (int)ui->mini_win0.y + cy - ui->mini_cur0[1]);
    }
}

/* Taking control shrinks the window to the mini strip, once: it stays that way, afterwards too,
   until the user asks for the full window, even while still in control. */
void ui_mini_follow_control(struct wye_ui *ui)
{
    static bool was_controlling;
    bool fwd = ui->st.mode != WYE_FWD_OFF;
    if (fwd && !was_controlling && ui->cfg.hud_mode == WYE_HUD_FORWARDING && !ui->hidden) {
        ui_mini_enter(ui);
    }
    was_controlling = fwd;
}
