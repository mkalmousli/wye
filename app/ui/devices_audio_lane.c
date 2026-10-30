/* Sound tab: one direction of sound as a lane - where it comes from, where it goes, how loud, and its switch. */
#include "devices.h"
#include "ui_fx.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include <math.h>

static void end(struct wye_ui *ui, Vector2 c, enum ui_icon icon, Color tint, const char *who, const char *device,
                bool right)
{
    ui_glow(c, 40, tint, 0.16f);
    DrawCircleV(c, 24, UI_CARD_HI);
    DrawRing(c, 22.5f, 24, 0, 360, 48, Fade(tint, 0.8f));
    ui_icon(icon, c, tint, true);
    Vector2 w = ui_measure(who, UI_FONT, true), d = ui_measure(device, UI_FONT_SMALL, false);
    float wx = right ? c.x - 36 - w.x : c.x + 36, dx = right ? c.x - 36 - d.x : c.x + 36;
    ui_text_bold(ui, who, wx, c.y - 21, UI_FONT, UI_TEXT);
    ui_text(ui, device, dx, c.y + 2, UI_FONT_SMALL, UI_MUTED);
}

/* Pulses flowing from a to b, brighter and faster with the sound; a still dashed line while off. */
static void flow(Vector2 a, Vector2 b, Color from, Color to, bool on, float level, uint32_t id)
{
    float len = b.x - a.x;
    if (!on) {
        for (float x = a.x; x < b.x; x += 12) {
            DrawLineEx((Vector2){x, a.y}, (Vector2){fminf(x + 6, b.x), a.y}, 2, UI_CARD_EDGE);
        }
        return;
    }
    float loud = fminf(1, sqrtf(level) * 1.4f);
    ui_gradient_bar((Rectangle){a.x, a.y - 1.5f, len, 3}, Fade(from, 0.5f), Fade(to, 0.5f));
    float phase = fmodf((float)GetTime() * (0.3f + 0.9f * loud) + (float)(id % 97) / 97.0f, 1.0f);
    for (int k = 0; k < 6; ++k) {
        float t = fmodf(phase + (float)k / 6.0f, 1.0f);
        Vector2 p = {a.x + len * t, a.y};
        ui_glow(p, 10 + 10 * loud, ui_mix(from, to, t), 0.25f + 0.45f * loud);
        DrawCircleV(p, 2.5f + 2 * loud, Fade(WHITE, 0.5f + 0.5f * loud));
    }
}

bool devices_audio_lane(struct wye_ui *ui, Rectangle r, const struct devices_lane *l)
{
    float hot = ui_tween(ui_id(l->title, r.x, r.y), CheckCollisionPointRec(GetMousePosition(), r) ? 1.0f : 0, 10);
    DrawRectangleRounded(r, 14.0f / r.height, 10, ui_mix(UI_BG2, UI_CARD_HI, 0.35f + 0.4f * hot));
    ui_text_bold(ui, l->title, r.x + 20, r.y + 14, UI_FONT_SMALL, l->on ? l->from_tint : UI_MUTED);
    ui_text(ui, l->state, r.x + 20 + ui_measure(l->title, UI_FONT_SMALL, true).x + 14, r.y + 14, UI_FONT_SMALL,
            UI_MUTED);
    bool value = l->on;
    bool flipped = ui_toggle((Rectangle){r.x + r.width - 64, r.y + 6, 44, UI_ROW}, "", &value);
    float cy = r.y + r.height / 2 + 12;
    Vector2 a = {r.x + 46, cy}, b = {r.x + r.width - 46, cy};
    end(ui, a, l->from_icon, l->on ? l->from_tint : UI_MUTED, l->from_who, l->from_device, false);
    end(ui, b, l->to_icon, l->on ? l->to_tint : UI_MUTED, l->to_who, l->to_device, true);
    float fa = a.x + 44 + fmaxf(ui_measure(l->from_device, UI_FONT_SMALL, false).x,
                                ui_measure(l->from_who, UI_FONT, true).x);
    float fb = b.x - 44 - fmaxf(ui_measure(l->to_device, UI_FONT_SMALL, false).x, ui_measure(l->to_who, UI_FONT, true).x);
    if (fb - fa > 40) {
        flow((Vector2){fa, cy}, (Vector2){fb, cy}, l->from_tint, l->to_tint, l->running, l->level,
             ui_id(l->title, 0, 0));
    }
    return flipped;
}
