/* Home > Live: the pictures - pointer pad, traffic graph and sound meters. */
#include "ui.h"
#include "ui_fx.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include <math.h>

/* Where the mouse is heading: a dot pulled out from the middle by the motion. */
void activity_pad(Rectangle r, float vx, float vy)
{
    Vector2 c = {r.x + r.width / 2, r.y + r.height / 2};
    float rad = fminf(r.width, r.height) / 2;
    DrawCircleV(c, rad, Fade(UI_BG2, 0.8f));
    DrawRing(c, rad - 1.5f, rad, 0, 360, 48, UI_CARD_EDGE);
    DrawRing(c, rad * 0.5f - 1, rad * 0.5f, 0, 360, 36, Fade(UI_CARD_EDGE, 0.6f));
    float len = sqrtf(vx * vx + vy * vy), scale = len > 1 ? fminf(rad - 8, len) / len : 0;
    Vector2 tip = {c.x + vx * scale, c.y + vy * scale};
    DrawLineEx(c, tip, 3, Fade(UI_ACCENT2, 0.6f));
    ui_glow(tip, 16, UI_ACCENT2, 0.5f);
    DrawCircleV(tip, 5, WHITE);
}

/* Messages per second to the other computer, as a filled line. */
void activity_graph(Rectangle r, const float *rate, int count, int head)
{
    float peak = 20;
    for (int i = 0; i < count; ++i) {
        peak = fmaxf(peak, rate[i]);
    }
    DrawLineEx((Vector2){r.x, r.y + r.height}, (Vector2){r.x + r.width, r.y + r.height}, 1, UI_CARD_EDGE);
    float w = r.width / (float)(count - 1);
    for (int i = 0; i < count - 1; ++i) {
        float a = rate[(head + i) % count] / peak, b = rate[(head + i + 1) % count] / peak;
        Vector2 p = {r.x + (float)i * w, r.y + r.height * (1 - a)}, q = {p.x + w, r.y + r.height * (1 - b)};
        Color fill = Fade(UI_ACCENT, 0.14f);
        DrawTriangle(p, (Vector2){p.x, r.y + r.height}, q, fill);
        DrawTriangle(q, (Vector2){p.x, r.y + r.height}, (Vector2){q.x, r.y + r.height}, fill);
        DrawLineEx(p, q, 2, UI_ACCENT);
    }
}

/* One sound direction: icon, name and a level bar that moves with the sound. */
static void meter(struct wye_ui *ui, Rectangle r, enum ui_icon icon, const char *name,
                  const struct wye_route_status *s, Color tint)
{
    Color col = s->running ? tint : UI_MUTED;
    ui_icon(icon, (Vector2){r.x + 9, r.y + r.height / 2}, col, s->running);
    ui_text(ui, name, r.x + 26, r.y + (r.height - 17) / 2, UI_FONT_SMALL, s->running ? UI_TEXT : UI_MUTED);
    Rectangle bar = {r.x + 112, r.y + r.height / 2 - 3, r.width - 112, 6};
    DrawRectangleRounded(bar, 1.0f, 6, UI_CARD_EDGE);
    if (s->running) {
        float shown = ui_tween(ui_id(name, r.x, r.y), sqrtf(fminf(1, s->level)), 20);
        ui_gradient_bar((Rectangle){bar.x, bar.y, fmaxf(6, bar.width * shown), bar.height}, tint, UI_ACCENT2);
    }
}

void activity_sound(struct wye_ui *ui, Rectangle r)
{
    const struct wye_audio_status *a = &ui->st.audio;
    meter(ui, (Rectangle){r.x, r.y, r.width, 22}, UI_ICON_SPEAKER, "Speaker", &a->route[WYE_ROUTE_SPEAKER],
          UI_TARGET);
    meter(ui, (Rectangle){r.x, r.y + 28, r.width, 22}, UI_ICON_MIC, "Microphone", &a->route[WYE_ROUTE_MIC],
          UI_TARGET2);
}
