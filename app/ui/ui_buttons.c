/*
 * Buttons and switches, drawn by hand: hover brightens and lifts, pressing
 * sinks, filled buttons glow. They respect GuiDisable()/GuiEnable().
 */
#include "ui.h"
#include "ui_fx.h"
#include "ui_theme.h"

#include "raygui.h"

static bool disabled(void)
{
    return GuiGetState() == STATE_DISABLED;
}

/* Hover (0..1) and whether the click landed this frame. */
static float interact(Rectangle r, const char *label, bool *clicked)
{
    bool hot = !disabled() && CheckCollisionPointRec(GetMousePosition(), r);
    *clicked = hot && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    return ui_tween(ui_id(label, r.x, r.y), hot ? 1.0f : 0.0f, 14);
}

static void label_in(Rectangle r, const char *label, Color col)
{
    Vector2 m = ui_measure(label, UI_FONT_SMALL, true);
    ui_draw_text(label, r.x + (r.width - m.x) / 2, r.y + (r.height - m.y) / 2, UI_FONT_SMALL, true, col);
}

static Rectangle pressed(Rectangle r)
{
    bool down = !disabled() && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r);
    return down ? (Rectangle){r.x + 1, r.y + 1, r.width - 2, r.height - 2} : r;
}

bool ui_button(Rectangle r, const char *label)
{
    bool clicked;
    float hot = interact(r, label, &clicked);
    Rectangle b = pressed(r);
    float round = 0.5f * 20 / b.height;
    DrawRectangleRounded(b, round, 8, disabled() ? UI_CARD : ui_mix(UI_CARD_HI, UI_CARD_EDGE, hot));
    DrawRectangleRoundedLinesEx(b, round, 8, 1, ui_mix(UI_CARD_EDGE, UI_ACCENT, hot * 0.7f));
    label_in(b, label, disabled() ? UI_MUTED : UI_TEXT);
    return clicked;
}

bool ui_button_tinted(Rectangle r, const char *label, Color tint)
{
    bool clicked;
    float hot = interact(r, label, &clicked);
    Rectangle b = pressed(r);
    float round = 0.5f * 20 / b.height;
    if (disabled()) {
        DrawRectangleRounded(b, round, 8, UI_CARD);
        DrawRectangleRoundedLinesEx(b, round, 8, 1, UI_CARD_EDGE);
        label_in(b, label, UI_MUTED);
        return false;
    }
    if (hot > 0.01f) {
        ui_glow((Vector2){b.x + b.width / 2, b.y + b.height / 2}, b.width * 0.5f, tint, 0.16f * hot);
    }
    DrawRectangleRounded(b, round, 8, ui_mix(tint, WHITE, hot * 0.12f));
    DrawRectangleRounded((Rectangle){b.x + 2, b.y + 1, b.width - 4, b.height * 0.45f}, round, 8, Fade(WHITE, 0.08f));
    label_in(b, label, WHITE);
    return clicked;
}

bool ui_button_primary(Rectangle r, const char *label)
{
    return ui_button_tinted(r, label, UI_ACCENT);
}

/* A sliding switch with its label; true when flipped this frame. */
bool ui_toggle(Rectangle r, const char *label, bool *value)
{
    Rectangle track = {r.x, r.y + (r.height - 22) / 2, 40, 22};
    Rectangle hit = {r.x, r.y, 52 + ui_measure(label, UI_FONT, false).x, r.height};
    bool clicked;
    interact(hit, label, &clicked);
    if (clicked) {
        *value = !*value;
    }
    float on = ui_tween(ui_id(label, r.x, r.y + 1), *value ? 1.0f : 0.0f, 16);
    Color fill = disabled() ? UI_CARD_EDGE : ui_mix(UI_CARD_EDGE, UI_OK, on);
    DrawRectangleRounded(track, 1.0f, 12, fill);
    Vector2 knob = {track.x + 11 + on * 18, track.y + 11};
    DrawCircleV((Vector2){knob.x, knob.y + 1}, 8.5f, Fade(BLACK, 0.3f));
    DrawCircleV(knob, 8.5f, disabled() ? UI_MUTED : WHITE);
    ui_draw_text(label, r.x + 52, r.y + (r.height - ui_measure(label, UI_FONT, false).y) / 2, UI_FONT, false,
                 disabled() ? UI_MUTED : UI_TEXT);
    return clicked;
}
