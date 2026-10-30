/* A segmented tab bar with a sliding highlight. */
#include "ui.h"
#include "ui_fx.h"
#include "ui_theme.h"

/* Draws the tabs; returns the (possibly newly) selected index. */
int ui_tabs(struct wye_ui *ui, Rectangle r, const char *const *labels, int count, int selected)
{
    float w = r.width / (float)count;
    DrawRectangleRounded(r, 1.0f, 12, UI_CARD);
    DrawRectangleRoundedLinesEx(r, 1.0f, 12, 1, UI_CARD_EDGE);
    float x = ui_tween(ui_id("tabs", r.x, r.y), r.x + w * (float)selected, 14);
    Rectangle pill = {x + 3, r.y + 3, w - 6, r.height - 6};
    ui_glow((Vector2){pill.x + pill.width / 2, pill.y + pill.height}, w * 0.5f, UI_ACCENT, 0.2f);
    DrawRectangleRounded(pill, 1.0f, 12, UI_ACCENT);
    for (int i = 0; i < count; ++i) {
        Rectangle t = {r.x + w * (float)i, r.y, w, r.height};
        bool hot = CheckCollisionPointRec(GetMousePosition(), t);
        Vector2 m = ui_measure(labels[i], UI_FONT_SMALL, true);
        ui_text_bold(ui, labels[i], t.x + (w - m.x) / 2, t.y + (t.height - m.y) / 2, UI_FONT_SMALL,
                     i == selected ? WHITE : hot ? UI_TEXT : UI_MUTED);
        if (hot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selected = i;
        }
    }
    return selected;
}
