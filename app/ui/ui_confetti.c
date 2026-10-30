/* A burst of confetti for happy moments (firmware installed, setup done). */
#include "setup_flash.h"
#include "ui_theme.h"
#include <math.h>

#define PIECES 160
#define GRAVITY 900.0f
#define LIFE_S 2.6f

static struct piece {
    Vector2 pos, vel;
    float angle, spin, age;
    Color color;
} g_pieces[PIECES];
static bool g_active;

void ui_confetti_burst(Vector2 at)
{
    static const Color COLORS[] = {UI_ACCENT, UI_OK, UI_WARN, UI_TARGET, {255, 120, 160, 255}};
    for (int i = 0; i < PIECES; ++i) {
        float a = (float)GetRandomValue(200, 340) * DEG2RAD; /* mostly upwards */
        float v = (float)GetRandomValue(300, 900);
        g_pieces[i] = (struct piece){
            .pos = at,
            .vel = {cosf(a) * v, sinf(a) * v},
            .angle = (float)GetRandomValue(0, 360),
            .spin = (float)GetRandomValue(-720, 720),
            .age = -(float)GetRandomValue(0, 150) / 1000.0f, /* staggered start */
            .color = COLORS[i % 5],
        };
    }
    g_active = true;
}

void ui_confetti_draw(void)
{
    if (!g_active) {
        return;
    }
    float dt = GetFrameTime() > 0.05f ? 0.05f : GetFrameTime();
    g_active = false;
    for (int i = 0; i < PIECES; ++i) {
        struct piece *p = &g_pieces[i];
        p->age += dt;
        if (p->age < 0 || p->age > LIFE_S) {
            g_active = g_active || p->age < 0;
            continue;
        }
        g_active = true;
        p->vel.y += GRAVITY * dt;
        p->vel.x *= 0.99f;
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        p->angle += p->spin * dt;
        float fade = p->age > LIFE_S - 0.6f ? (LIFE_S - p->age) / 0.6f : 1.0f;
        float flip = fabsf(cosf(p->angle * DEG2RAD * 2)); /* tumbling paper */
        DrawRectanglePro((Rectangle){p->pos.x, p->pos.y, 10, 6 * flip + 1}, (Vector2){5, 3}, p->angle,
                         Fade(p->color, fade));
    }
}
