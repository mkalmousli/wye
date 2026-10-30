/* Home link map: a node (computer or Pico) in a glowing ring, with its title and state. */
#include "home_map.h"
#include "ui_fx.h"
#include "ui_theme.h"

static Color state_color(enum map_state s)
{
    return s == MAP_OK ? UI_OK : s == MAP_WARN ? UI_WARN : s == MAP_BAD ? UI_BAD : UI_MUTED;
}

static void computer(Vector2 c, Color col)
{
    DrawRectangleRoundedLinesEx((Rectangle){c.x - 13, c.y - 11, 26, 17}, 0.15f, 4, 2, col);
    DrawRectangleRounded((Rectangle){c.x - 16, c.y + 8, 32, 4}, 1.0f, 4, col);
}

static void pico(Vector2 c, Color col)
{
    DrawRectangleRounded((Rectangle){c.x - 9, c.y - 14, 18, 28}, 0.2f, 4, Fade(col, 0.25f));
    DrawRectangleRoundedLinesEx((Rectangle){c.x - 9, c.y - 14, 18, 28}, 0.2f, 4, 2, col);
    for (int i = 0; i < 5; ++i) { /* pins */
        float y = c.y - 10 + (float)i * 5;
        DrawRectangle((int)c.x - 12, (int)y, 3, 2, col);
        DrawRectangle((int)c.x + 9, (int)y, 3, 2, col);
    }
    DrawRectangle((int)c.x - 4, (int)c.y - 3, 8, 8, col);
}

void map_node_draw(struct wye_ui *ui, const struct map_node *n, Vector2 at, bool controlled)
{
    Color col = controlled ? UI_TARGET : state_color(n->state);
    float beat = controlled ? ui_pulse(1.0f) : n->state == MAP_OK ? 0.3f : 0.0f;
    ui_glow(at, 46 + 8 * beat, col, 0.16f + 0.18f * beat);
    DrawCircleV(at, 30, UI_CARD_HI);
    DrawRing(at, 28, 31, 0, 360, 48, n->state == MAP_OFF && !controlled ? UI_CARD_EDGE : col);
    Color glyph = n->state == MAP_OFF ? UI_MUTED : n->glyph.a ? n->glyph : UI_TEXT;
    (n->kind == MAP_COMPUTER ? computer : pico)(at, glyph);
    Vector2 m = ui_measure(n->title, UI_FONT_SMALL, true);
    ui_text_bold(ui, n->title, at.x - m.x / 2, at.y + 38, UI_FONT_SMALL, UI_TEXT);
    m = ui_measure(n->detail, 12, false);
    ui_text(ui, n->detail, at.x - m.x / 2, at.y + 56, 12, n->state == MAP_OK ? UI_MUTED : col);
}

void map_leds(struct wye_ui *ui, Vector2 at, uint8_t leds)
{
    static const char *const NAMES[] = {"Num", "Caps", "Scroll"};
    float x = at.x + 52 - 186; /* right-aligned under the node, inside the card */
    for (int i = 0; i < 3; ++i) { /* the other keyboard's lock lights */
        bool on = (leds >> i) & 1;
        float w = ui_measure(NAMES[i], 13, false).x + 26;
        DrawRectangleRounded((Rectangle){x, at.y + 8, w, 22}, 1.0f, 8, Fade(on ? UI_OK : UI_CARD_EDGE, 0.25f));
        if (on) {
            ui_glow((Vector2){x + 10, at.y + 19}, 9, UI_OK, 0.6f);
        }
        DrawCircleV((Vector2){x + 10, at.y + 19}, 3, on ? UI_OK : UI_MUTED);
        ui_text(ui, NAMES[i], x + 18, at.y + 11, 13, on ? UI_TEXT : UI_MUTED);
        x += w + 5;
    }
}

const char *map_version(const struct wye_node_info *n)
{
    return TextFormat("firmware %u.%u.%u", n->hello.fw_major, n->hello.fw_minor, n->hello.fw_patch);
}
