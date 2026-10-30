/* Devices > Overview: one function of the other computer as a tile - icon, switch, what it does, live state. */
#include "devices.h"
#include "ui_fx.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include "raygui.h"

#include <math.h>

/* A level meter: segments that light up with the sound, like a mixing desk. */
static void meter(Rectangle r, float level, Color tint)
{
    const int n = 18;
    float w = (r.width - (float)(n - 1) * 3) / (float)n, lit = sqrtf(fminf(1, level)) * (float)n;
    for (int i = 0; i < n; ++i) {
        Rectangle s = {r.x + (float)i * (w + 3), r.y, w, r.height};
        Color c = i >= n - 3 ? UI_WARN : tint;
        DrawRectangleRounded(s, 0.5f, 4, (float)i < lit ? c : Fade(UI_CARD_EDGE, 0.8f));
    }
}

static void wrap2(struct wye_ui *ui, const char *text, float x, float y, float width)
{
    char line[128];
    int start = 0, last_space = -1, row = 0;
    for (int i = 0;; ++i) {
        bool end = text[i] == '\0';
        if (text[i] == ' ' || end) {
            snprintf(line, sizeof(line), "%.*s", i - start, text + start);
            if (ui_measure(line, UI_FONT_SMALL, false).x > width && last_space > start) {
                snprintf(line, sizeof(line), "%.*s", last_space - start, text + start);
                ui_text(ui, line, x, y + (float)row++ * 20, UI_FONT_SMALL, UI_MUTED);
                start = last_space + 1;
            }
            last_space = i;
        }
        if (end) {
            ui_text(ui, text + start, x, y + (float)row * 20, UI_FONT_SMALL, UI_MUTED);
            return;
        }
    }
}

bool devices_tile(struct wye_ui *ui, Rectangle r, const struct devices_tile *t)
{
    float on = ui_tween(ui_id(t->name, r.x, r.y), t->on ? 1.0f : 0.0f, 10);
    Color tint = ui_mix(UI_MUTED, t->tint, on);
    if (on > 0.01f) {
        ui_glow((Vector2){r.x + 46, r.y + 46}, 80, t->tint, 0.10f * on);
    }
    ui_card(ui, r, NULL);
    DrawRectangleRoundedLinesEx(r, 16.0f / fminf(r.width, r.height), 10, 1.2f, Fade(t->tint, 0.45f * on));
    Vector2 ic = {r.x + 42, r.y + 42};
    DrawCircleV(ic, 22, Fade(tint, 0.10f + 0.12f * on));
    ui_icon(t->icon, ic, tint, t->on);
    bool value = t->on, flipped = false;
    if (!t->available) {
        GuiDisable();
    }
    flipped = ui_toggle((Rectangle){r.x + r.width - 60, r.y + 24, 44, UI_ROW}, "", &value);
    GuiEnable();
    ui_text_bold(ui, t->name, r.x + 20, r.y + 80, 18, t->on ? UI_TEXT : UI_MUTED);
    wrap2(ui, t->about, r.x + 20, r.y + 108, r.width - 40);
    float y = r.y + r.height - 44;
    if (t->level >= 0 && t->on) {
        meter((Rectangle){r.x + 20, y - 14, r.width - 40, 8}, t->level, t->tint);
    }
    ui_text(ui, t->status, r.x + 20, y + 4, UI_FONT_SMALL, t->on ? t->tint : UI_MUTED);
    return flipped;
}
