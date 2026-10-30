/* Eased per-widget values: hover glows, sliding indicators, switch knobs. */
#include "ui_fx.h"
#include <math.h>

#define SLOTS 512

static struct {
    uint32_t id;
    float value;
    bool used;
} g[SLOTS];

uint32_t ui_id(const char *label, float x, float y)
{
    uint32_t h = 2166136261u;
    for (const char *p = label ? label : ""; *p; ++p) {
        h = (h ^ (uint8_t)*p) * 16777619u;
    }
    h = (h ^ (uint32_t)(int)x) * 16777619u;
    return ((h ^ (uint32_t)(int)y) * 16777619u) | 1u;
}

float ui_tween(uint32_t id, float target, float speed)
{
    uint32_t i = id % SLOTS;
    for (int probe = 0; probe < SLOTS && g[i].used && g[i].id != id; ++probe) {
        i = (i + 1) % SLOTS;
    }
    if (!g[i].used || g[i].id != id) { /* first sight (or table full): start at the target */
        g[i].used = true;
        g[i].id = id;
        g[i].value = target;
    }
    float dt = GetFrameTime() > 0.1f ? 0.1f : GetFrameTime();
    g[i].value += (target - g[i].value) * (1.0f - expf(-speed * dt));
    return g[i].value;
}

float ui_pulse(float hz)
{
    return 0.5f + 0.5f * sinf((float)GetTime() * hz * 6.2831853f);
}

Color ui_mix(Color a, Color b, float t)
{
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    return (Color){(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                   (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t)};
}
