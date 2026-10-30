/* Choice controls drawn in the app's own style: segmented pills and sliders. */
#include "ui.h"
#include "ui_fx.h"
#include "ui_theme.h"

#include "raygui.h"

#include <math.h>
#include <string.h>

#define MAX_ITEMS 8

static bool disabled(void) { return GuiGetState() == STATE_DISABLED; }

/* One pill holding all choices ("A;B;C"); a highlight slides to the chosen one. */
int ui_segmented(Rectangle r, const char *items, int selected)
{
    char buf[256], *names[MAX_ITEMS];
    int n = 0;
    strncpy(buf, items, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    for (char *p = strtok(buf, ";"); p && n < MAX_ITEMS; p = strtok(NULL, ";")) {
        names[n++] = p;
    }
    if (n == 0) {
        return selected;
    }
    float w = r.width / (float)n, round = 1.0f;
    DrawRectangleRounded(r, round, 12, UI_BG2);
    DrawRectangleRoundedLinesEx(r, round, 12, 1, UI_CARD_EDGE);
    float at = ui_tween(ui_id(items, r.x, r.y), (float)selected, 18);
    Rectangle pill = {r.x + 3 + at * w, r.y + 3, w - 6, r.height - 6};
    if (selected >= 0) {
        DrawRectangleRounded(pill, round, 12, disabled() ? UI_CARD_EDGE : UI_ACCENT);
        DrawRectangleRounded((Rectangle){pill.x + 2, pill.y + 1, pill.width - 4, pill.height * 0.45f}, round, 12,
                             Fade(WHITE, 0.08f));
    }
    for (int i = 0; i < n; ++i) {
        Rectangle cell = {r.x + (float)i * w, r.y, w, r.height};
        bool hot = !disabled() && CheckCollisionPointRec(GetMousePosition(), cell);
        if (hot && i != selected && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            selected = i;
        }
        Vector2 m = ui_measure(names[i], UI_FONT_SMALL, true);
        Color col = i == selected ? WHITE : hot ? UI_TEXT : UI_MUTED;
        ui_draw_text(names[i], cell.x + (w - m.x) / 2, cell.y + (r.height - m.y) / 2, UI_FONT_SMALL, true,
                     disabled() ? UI_MUTED : col);
    }
    return selected;
}

/* A track with a glowing fill and a round knob; true while the value changes. */
bool ui_slider(Rectangle r, float *value, float lo, float hi)
{
    static uint32_t dragging;
    uint32_t id = ui_id("slider", r.x, r.y);
    Vector2 mouse = GetMousePosition();
    Rectangle hit = {r.x - 8, r.y - 10, r.width + 16, r.height + 20};
    if (!disabled() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, hit)) {
        dragging = id;
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        dragging = dragging == id ? 0 : dragging;
    }
    float old = *value;
    if (dragging == id) {
        float t = fminf(1, fmaxf(0, (mouse.x - r.x) / r.width));
        *value = lo + t * (hi - lo);
    }
    float t = (*value - lo) / (hi - lo), cy = r.y + r.height / 2;
    float hot = ui_tween(id, dragging == id ? 1.0f : CheckCollisionPointRec(mouse, hit) ? 0.5f : 0.0f, 14);
    DrawRectangleRounded((Rectangle){r.x, cy - 3, r.width, 6}, 1.0f, 8, UI_CARD_EDGE);
    if (!disabled()) {
        ui_gradient_bar((Rectangle){r.x, cy - 3, fmaxf(6, r.width * t), 6}, UI_ACCENT, UI_ACCENT2);
    }
    Vector2 knob = {r.x + r.width * t, cy};
    if (hot > 0.01f) {
        DrawCircleV(knob, 11 + 4 * hot, Fade(UI_ACCENT, 0.18f * hot));
    }
    DrawCircleV((Vector2){knob.x, knob.y + 1.5f}, 9, Fade(BLACK, 0.35f));
    DrawCircleV(knob, 9, disabled() ? UI_MUTED : WHITE);
    return *value != old;
}
