/* Light and depth: background, shadows, glows, gradient bars, keycaps. */
#include "ui.h"
#include "ui_fx.h"
#include "ui_theme.h"

void ui_background(void)
{
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangleGradientV(0, 0, w, h, UI_BG, UI_BG2);
    ui_glow((Vector2){(float)w * 0.85f, -40}, (float)w * 0.45f, UI_ACCENT, 0.07f); /* soft light, top right */
    ui_glow((Vector2){(float)w * 0.15f, (float)h + 60}, (float)w * 0.35f, UI_TARGET, 0.05f);
}

void ui_shadow(Rectangle r, float roundness)
{
    for (int i = 3; i >= 1; --i) {
        float s = (float)i * 3;
        DrawRectangleRounded((Rectangle){r.x - s / 2, r.y + s / 2, r.width + s, r.height + s}, roundness, 8,
                             Fade(BLACK, 0.09f));
    }
}

void ui_glow(Vector2 c, float radius, Color col, float strength)
{
    if (radius > 0 && strength > 0) {
        DrawCircleGradient((int)c.x, (int)c.y, radius, Fade(col, strength), Fade(col, 0));
    }
}

void ui_gradient_bar(Rectangle r, Color from, Color to)
{
    if (r.width <= 0) {
        return;
    }
    float rad = r.height / 2;
    DrawCircleV((Vector2){r.x + rad, r.y + rad}, rad, from);
    DrawCircleV((Vector2){r.x + r.width - rad, r.y + rad}, rad, to);
    if (r.width > 2 * rad) {
        DrawRectangleGradientH((int)(r.x + rad), (int)r.y, (int)(r.width - 2 * rad), (int)r.height, from, to);
    }
}

void ui_keycap(struct wye_ui *ui, Rectangle r, const char *label, bool lit, Color tint)
{
    Color face = lit ? tint : UI_CARD_HI, lip = lit ? ui_mix(tint, BLACK, 0.35f) : UI_CARD_EDGE;
    if (lit) {
        ui_glow((Vector2){r.x + r.width / 2, r.y + r.height / 2}, r.width * 0.8f, tint, 0.22f);
    }
    DrawRectangleRounded((Rectangle){r.x, r.y + 3, r.width, r.height}, 0.3f, 6, lip);
    DrawRectangleRounded(r, 0.3f, 6, face);
    float size = r.height * 0.42f;
    Vector2 m = ui_measure(label, size, true);
    if (m.x > r.width - 8) { /* long names shrink to fit */
        size *= (r.width - 8) / m.x;
        m = ui_measure(label, size, true);
    }
    ui_text_bold(ui, label, r.x + (r.width - m.x) / 2, r.y + (r.height - m.y) / 2, size, lit ? WHITE : UI_TEXT);
}
