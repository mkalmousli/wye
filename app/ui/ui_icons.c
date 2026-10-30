/* Small line icons drawn with raylib shapes (no image assets), 16 px nominal. */
#include "ui_icons.h"
#include <math.h>

static void monitor(Vector2 c, Color col, bool filled)
{
    Rectangle screen = {c.x - 8, c.y - 7, 16, 11};
    if (filled) {
        DrawRectangleRounded(screen, 0.2f, 4, col);
    } else {
        DrawRectangleRoundedLinesEx(screen, 0.2f, 4, 1.6f, col);
    }
    DrawLineEx((Vector2){c.x, c.y + 4}, (Vector2){c.x, c.y + 7}, 1.6f, col);
    DrawLineEx((Vector2){c.x - 4, c.y + 7.5f}, (Vector2){c.x + 4, c.y + 7.5f}, 1.6f, col);
}

static void mouse(Vector2 c, Color col, bool active)
{
    float wob = active ? 2.0f * sinf((float)GetTime() * 12) : 0;
    Rectangle body = {c.x - 4.5f + wob, c.y - 7, 9, 14};
    DrawRectangleRoundedLinesEx(body, 1.0f, 8, 1.6f, col);
    DrawLineEx((Vector2){body.x + 4.5f, body.y + 1}, (Vector2){body.x + 4.5f, body.y + 5}, 1.4f, col);
    for (int i = -1; i <= 1; i += 2) { /* motion marks */
        float x = c.x + (float)i * 8.5f + wob;
        DrawLineEx((Vector2){x, c.y - 3}, (Vector2){x, c.y + 3}, 1.4f, Fade(col, active ? 1.0f : 0.5f));
    }
}

static void clipboard(Vector2 c, Color col)
{
    DrawRectangleRoundedLinesEx((Rectangle){c.x - 5.5f, c.y - 6, 11, 14}, 0.2f, 4, 1.6f, col);
    DrawRectangleRounded((Rectangle){c.x - 3, c.y - 8, 6, 4}, 0.4f, 4, col);
    DrawLineEx((Vector2){c.x - 3, c.y}, (Vector2){c.x + 3, c.y}, 1.4f, col);
    DrawLineEx((Vector2){c.x - 3, c.y + 3.5f}, (Vector2){c.x + 2, c.y + 3.5f}, 1.4f, col);
}

static void folder(Vector2 c, Color col)
{
    DrawRectangleRounded((Rectangle){c.x - 8, c.y - 6, 7, 4}, 0.4f, 4, col);
    DrawRectangleRoundedLinesEx((Rectangle){c.x - 8, c.y - 4, 16, 11}, 0.2f, 4, 1.6f, col);
}

static void expand(Vector2 c, Color col)
{
    DrawRectangleRoundedLinesEx((Rectangle){c.x - 6, c.y - 6, 12, 12}, 0.15f, 4, 1.6f, col);
    DrawLineEx((Vector2){c.x - 1, c.y + 1}, (Vector2){c.x + 4, c.y - 4}, 1.6f, col);
}

void ui_icon(enum ui_icon icon, Vector2 c, Color col, bool active)
{
    switch (icon) {
    case UI_ICON_CONTROL: monitor(c, col, active); break;
    case UI_ICON_JIGGLE: mouse(c, col, active); break;
    case UI_ICON_PASTE: clipboard(c, col); break;
    case UI_ICON_FOLDER: folder(c, col); break;
    case UI_ICON_EXPAND: expand(c, col); break;
    default:
        ui_icon_nav(icon, c, col);
        break;
    case UI_ICON_CLOSE:
        DrawLineEx((Vector2){c.x - 5, c.y - 5}, (Vector2){c.x + 5, c.y + 5}, 1.8f, col);
        DrawLineEx((Vector2){c.x - 5, c.y + 5}, (Vector2){c.x + 5, c.y - 5}, 1.8f, col);
        break;
    }
}

/* Hover highlight + icon; returns true when clicked. *hovered is set while the pointer is over it. */
bool ui_icon_button(Rectangle r, enum ui_icon icon, Color col, bool active, bool enabled, bool *hovered)
{
    bool hot = CheckCollisionPointRec(GetMousePosition(), r);
    if (hot) {
        DrawRectangleRounded(r, 0.3f, 6, (Color){255, 255, 255, 22});
        *hovered = true;
    }
    ui_icon(icon, (Vector2){r.x + r.width / 2, r.y + r.height / 2}, enabled ? col : Fade(col, 0.35f), active);
    return enabled && hot && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}
