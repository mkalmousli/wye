/* The navigation rail: logo, page icons with a sliding marker, live status light, mini mode. */
#include "ui.h"
#include "ui_fx.h"
#include "ui_icons.h"
#include "ui_theme.h"

struct item { enum wye_page page; enum ui_icon icon; const char *label; };

static const struct item TOP[] = {
    {PAGE_HOME, UI_ICON_HOME, "Home"},       {PAGE_ROUTINES, UI_ICON_ROUTINE, "Routines"},
    {PAGE_DEVICES, UI_ICON_KEYBOARD, "Devices"}, {PAGE_DEVICE, UI_ICON_CHIP, "Picos"},
    {PAGE_SETTINGS, UI_ICON_GEAR, "Settings"},
};
static const struct item ABOUT = {PAGE_ABOUT, UI_ICON_INFO, "About"};

const char *ui_status_text(const struct wye_ui *ui, Color *c)
{
    const struct wye_link_info *l = &ui->st.link;
    if (ui->st.mode != WYE_FWD_OFF) {
        *c = UI_TARGET;
        return "Controlling the other computer";
    }
    *c = l->state == WYE_LINK_READY ? UI_OK : l->state == WYE_LINK_BRIDGE ? UI_WARN : UI_BAD;
    return l->state == WYE_LINK_READY ? "Ready" : l->state == WYE_LINK_BRIDGE ? "Other computer not answering"
         : l->state == WYE_LINK_LEGACY ? "Firmware update needed" : "No Pico connected";
}

/* One icon button; returns its y if it is the current page (for the marker). */
static bool item(struct wye_ui *ui, const struct item *it, float y)
{
    Rectangle r = {8, y, UI_SIDEBAR_W - 16, 56};
    bool current = ui->page == it->page || (it->page == PAGE_DEVICES && ui->page == PAGE_IDENTITY);
    bool hot = CheckCollisionPointRec(GetMousePosition(), r);
    float glow = ui_tween(ui_id(it->label, 0, y), current ? 1.0f : hot ? 0.5f : 0.0f, 12);
    if (glow > 0.02f) {
        DrawRectangleRounded(r, 0.3f, 8, Fade(current ? UI_ACCENT : UI_CARD_HI, 0.22f * glow + (current ? 0.0f : 0.3f * glow)));
    }
    Color col = current ? UI_ACCENT : ui_mix(UI_MUTED, UI_TEXT, glow);
    ui_icon(it->icon, (Vector2){r.x + r.width / 2, r.y + 20}, col, current);
    Vector2 m = ui_measure(it->label, 11.5f, false);
    ui_text(ui, it->label, r.x + (r.width - m.x) / 2, r.y + 35, 11.5f, col);
    if (hot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        ui->page = it->page;
    }
    return current;
}

static void status_light(struct wye_ui *ui, float y)
{
    Color c;
    const char *text = ui_status_text(ui, &c);
    Vector2 at = {UI_SIDEBAR_W / 2, y};
    float beat = ui->st.mode != WYE_FWD_OFF ? ui_pulse(1.2f) : 0.4f;
    ui_glow(at, 22 + 6 * beat, c, 0.35f + 0.25f * beat);
    DrawCircleV(at, 6, c);
    if (CheckCollisionPointCircle(GetMousePosition(), at, 16)) { /* tooltip into the page area */
        Vector2 m = ui_measure(text, UI_FONT_SMALL, false);
        Rectangle tip = {UI_SIDEBAR_W + 4, y - 15, m.x + 24, 30};
        DrawRectangleRounded(tip, 0.4f, 8, UI_CARD_HI);
        ui_text(ui, text, tip.x + 12, tip.y + 7, UI_FONT_SMALL, UI_TEXT);
    }
}

/* The app icon's mark: one stem (your keyboard) forking to two computers. */
static void wye_logo(float x, float y)
{
    Vector2 fork = {x, y + 1}, stem = {x, y + 17}, left = {x - 14, y - 14}, right = {x + 14, y - 14};
    ui_glow(fork, 34, UI_ACCENT, 0.2f);
    DrawLineEx(fork, stem, 6, UI_TEXT);
    DrawLineEx(fork, left, 6, UI_ACCENT);
    DrawLineEx(fork, right, 6, UI_TARGET);
    DrawCircleV(left, 5.5f, UI_ACCENT);
    DrawCircleV(right, 5.5f, UI_TARGET);
    DrawCircleV(stem, 5.5f, UI_TEXT);
}

void ui_sidebar(struct wye_ui *ui)
{
    float h = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)UI_SIDEBAR_W, (int)h, UI_SIDEBAR);
    DrawLineEx((Vector2){UI_SIDEBAR_W, 0}, (Vector2){UI_SIDEBAR_W, h}, 1, UI_CARD_EDGE);
    wye_logo(UI_SIDEBAR_W / 2, 36);
    float marker = -1, y = 76;
    for (size_t i = 0; i < sizeof(TOP) / sizeof(TOP[0]); ++i, y += 60) {
        marker = item(ui, &TOP[i], y) ? y : marker;
    }
    y = h - 170;
    marker = item(ui, &ABOUT, y) ? y : marker;
    float shown = ui_tween(ui_id("rail-marker", 0, 0), marker, 16);
    if (marker >= 0) {
        DrawRectangleRounded((Rectangle){0, shown + 14, 4, 28}, 1.0f, 4, UI_ACCENT);
    }
    bool hot = false;
    if (ui_icon_button((Rectangle){UI_SIDEBAR_W / 2 - 16, h - 78, 32, 28}, UI_ICON_MINI, UI_MUTED, false, true, &hot)) {
        ui_mini_enter(ui);
    }
    status_light(ui, h - 30);
}
