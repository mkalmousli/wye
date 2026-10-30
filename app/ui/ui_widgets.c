/* Text, cards, status dots, progress bars and text boxes. */
#include "ui.h"
#include "ui_fx.h"
#include "ui_theme.h"

#include "raygui.h"

void ui_text(struct wye_ui *ui, const char *s, float x, float y, float size, Color c)
{
    (void)ui;
    ui_draw_text(s, x, y, size, false, c);
}

void ui_text_bold(struct wye_ui *ui, const char *s, float x, float y, float size, Color c)
{
    (void)ui;
    ui_draw_text(s, x, y, size, true, c);
}

/* A raised card; returns its content area (below the title, if any). */
Rectangle ui_card(struct wye_ui *ui, Rectangle r, const char *title)
{
    float round = 16.0f / (r.height < r.width ? r.height : r.width);
    ui_shadow(r, round);
    DrawRectangleRounded(r, round, 10, UI_CARD);
    DrawRectangleRoundedLinesEx(r, round, 10, 1.0f, UI_CARD_EDGE);
    DrawLineEx((Vector2){r.x + 16, r.y + 1}, (Vector2){r.x + r.width - 16, r.y + 1}, 1, Fade(WHITE, 0.05f));
    float top = r.y + UI_PAD;
    if (title) {
        ui_text_bold(ui, title, r.x + UI_PAD, top - 2, UI_FONT, UI_TEXT);
        top += UI_FONT + 14;
    }
    return (Rectangle){r.x + UI_PAD, top, r.width - 2 * UI_PAD, r.y + r.height - top - UI_PAD};
}

void ui_dot(float x, float y, Color c)
{
    ui_glow((Vector2){x, y}, 14, c, 0.45f);
    DrawCircleV((Vector2){x, y}, 5, c);
}

void ui_progress(Rectangle r, int percent, Color c)
{
    DrawRectangleRounded(r, 1.0f, 8, UI_CARD_EDGE);
    float shown = ui_tween(ui_id("progress", r.x, r.y), (float)(percent > 100 ? 100 : percent), 8);
    if (shown > 0.5f) {
        ui_gradient_bar((Rectangle){r.x, r.y, r.width * shown / 100.0f, r.height}, c, ui_mix(c, UI_ACCENT2, 0.6f));
    }
}

/* Labelled text box; click to edit, Enter or a click elsewhere ends it. True when an edit ends. */
bool ui_textbox(struct wye_ui *ui, Rectangle r, const char *label, char *buf, int size, int id, int *editing)
{
    if (label) {
        ui_text(ui, label, r.x, r.y - 20, UI_FONT_SMALL, UI_MUTED);
    }
    if (GuiTextBox(r, buf, size, *editing == id)) {
        bool ended = *editing == id;
        *editing = ended ? 0 : id;
        return ended;
    }
    return false;
}

float ui_chip_width(struct wye_ui *ui, const char *text)
{
    (void)ui;
    return ui_measure(text, UI_FONT_SMALL, false).x + 26;
}

float ui_chip(struct wye_ui *ui, float x, float y, const char *text, Color col)
{
    Rectangle r = {x, y, ui_chip_width(ui, text), 26};
    DrawRectangleRounded(r, 1.0f, 8, Fade(col, 0.14f));
    ui_text(ui, text, r.x + 13, r.y + 5, UI_FONT_SMALL, col);
    return r.width;
}
