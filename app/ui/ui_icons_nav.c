/* Navigation and action icons (16 px nominal), drawn with raylib shapes. */
#include "ui_icons.h"
#include <math.h>

static void line(float x1, float y1, float x2, float y2, Color c)
{
    DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 1.7f, c);
}

static void gear(Vector2 c, Color col)
{
    for (int i = 0; i < 8; ++i) {
        float a = (float)i * 0.785398f;
        line(c.x + cosf(a) * 5, c.y + sinf(a) * 5, c.x + cosf(a) * 8.5f, c.y + sinf(a) * 8.5f, col);
    }
    DrawRing(c, 3.5f, 5.8f, 0, 360, 24, col);
}

static void keyboard(Vector2 c, Color col)
{
    DrawRectangleRoundedLinesEx((Rectangle){c.x - 9, c.y - 6, 18, 12}, 0.25f, 4, 1.6f, col);
    for (int i = 0; i < 4; ++i) {
        DrawRectangle((int)(c.x - 6 + (float)i * 3.5f), (int)c.y - 3, 2, 2, col);
    }
    line(c.x - 5, c.y + 3, c.x + 5, c.y + 3, col);
}

void ui_icon_nav(enum ui_icon icon, Vector2 c, Color col)
{
    switch (icon) {
    case UI_ICON_HOME:
        line(c.x - 8, c.y - 1, c.x, c.y - 8, col);
        line(c.x, c.y - 8, c.x + 8, c.y - 1, col);
        DrawRectangleRoundedLinesEx((Rectangle){c.x - 5.5f, c.y - 2, 11, 9}, 0.15f, 4, 1.6f, col);
        break;
    case UI_ICON_ROUTINE: /* a play button in a circle: replay what was recorded */
        DrawRing(c, 7, 8.6f, 0, 360, 32, col);
        DrawTriangle((Vector2){c.x - 2.5f, c.y - 4}, (Vector2){c.x - 2.5f, c.y + 4}, (Vector2){c.x + 4, c.y}, col);
        break;
    case UI_ICON_KEYBOARD: keyboard(c, col); break;
    case UI_ICON_CHIP:
        DrawRectangleRoundedLinesEx((Rectangle){c.x - 6, c.y - 6, 12, 12}, 0.2f, 4, 1.6f, col);
        for (int i = -1; i <= 1; ++i) {
            line(c.x - 9, c.y + (float)i * 3.5f, c.x - 6, c.y + (float)i * 3.5f, col);
            line(c.x + 6, c.y + (float)i * 3.5f, c.x + 9, c.y + (float)i * 3.5f, col);
        }
        DrawRectangle((int)c.x - 2, (int)c.y - 2, 4, 4, col);
        break;
    case UI_ICON_GEAR: gear(c, col); break;
    case UI_ICON_INFO:
        DrawRing(c, 7, 8.6f, 0, 360, 32, col);
        line(c.x, c.y - 1, c.x, c.y + 4, col);
        DrawCircleV((Vector2){c.x, c.y - 4}, 1.2f, col);
        break;
    case UI_ICON_BOLT:
        DrawTriangle((Vector2){c.x + 2, c.y - 8}, (Vector2){c.x - 5, c.y + 1}, (Vector2){c.x + 1, c.y + 1}, col);
        DrawTriangle((Vector2){c.x - 1, c.y - 1}, (Vector2){c.x - 2, c.y + 8}, (Vector2){c.x + 5, c.y - 1}, col);
        break;
    case UI_ICON_MINI:
        DrawRectangleRoundedLinesEx((Rectangle){c.x - 8, c.y - 6, 16, 12}, 0.2f, 4, 1.6f, col);
        DrawRectangleRounded((Rectangle){c.x + 1, c.y + 0.5f, 5, 3.5f}, 0.3f, 4, col);
        break;
    case UI_ICON_SPEAKER:
        DrawRectangle((int)c.x - 8, (int)c.y - 3, 4, 6, col);
        DrawTriangle((Vector2){c.x - 4, c.y - 3}, (Vector2){c.x - 4, c.y + 3}, (Vector2){c.x + 1, c.y + 7}, col);
        DrawTriangle((Vector2){c.x - 4, c.y - 3}, (Vector2){c.x + 1, c.y + 7}, (Vector2){c.x + 1, c.y - 7}, col);
        DrawRing((Vector2){c.x + 1, c.y}, 4.5f, 6, -50, 50, 12, col);
        DrawRing((Vector2){c.x + 1, c.y}, 8, 9.5f, -50, 50, 12, col);
        break;
    case UI_ICON_MIC:
        DrawRectangleRounded((Rectangle){c.x - 3, c.y - 9, 6, 11}, 1.0f, 8, col);
        DrawRing((Vector2){c.x, c.y - 2}, 5.5f, 7, 0, 180, 16, col);
        line(c.x, c.y + 5, c.x, c.y + 8.5f, col);
        line(c.x - 3.5f, c.y + 8.5f, c.x + 3.5f, c.y + 8.5f, col);
        break;
    default:
        break;
    }
}
