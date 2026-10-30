/* Smooth, slightly curved pointer paths for the jiggler. */
#include "jiggler.h"

#include <stdlib.h>

int wye_jiggler_rand(struct wye_jiggler *j, int lo, int hi)
{
    j->rng = j->rng * 1664525u + 1013904223u; /* LCG: plenty for motion jitter */
    if (hi <= lo) {
        return lo;
    }
    return lo + (int)((j->rng >> 8) % (uint32_t)(hi - lo + 1));
}

void wye_jiggler_begin_leg(struct wye_jiggler *j, uint64_t now, enum wye_jiggle_phase phase,
                           int tx, int ty)
{
    int bend = wye_jiggler_rand(j, 45, 120) * (wye_jiggler_rand(j, 0, 1) ? 1 : -1);
    int wobble = wye_jiggler_rand(j, -35, 35);
    j->phase = phase;
    j->leg_start = now;
    j->leg_ms = wye_jiggler_rand(j, 1000, 2300);
    j->target_x = tx;
    j->target_y = ty;
    /* Bend perpendicular to the dominant direction of travel. */
    j->curve_x = abs(tx) >= abs(ty) ? wobble : bend;
    j->curve_y = abs(tx) >= abs(ty) ? bend : wobble;
    j->last_x = j->last_y = 0;
}

/* Smoothstep easing in thousandths. */
static int ease_1000(uint64_t elapsed, int duration)
{
    if (duration <= 0 || elapsed >= (uint64_t)duration) {
        return 1000;
    }
    long long t = (long long)elapsed * 1000 / duration;
    return (int)((t * t * (3000 - 2 * t) + 500000) / 1000000);
}

static int scale(int v, int s)
{
    long long r = (long long)v * s;
    return (int)(r >= 0 ? (r + 500) / 1000 : (r - 500) / 1000);
}

void wye_jiggler_step(struct wye_jiggler *j, uint64_t now, int *dx, int *dy)
{
    int e = ease_1000(now - j->leg_start, j->leg_ms);
    int arc = (int)((4LL * e * (1000 - e) + 500) / 1000);   /* 0 at ends, max mid-way */
    int x = scale(j->target_x, e) + scale(j->curve_x, arc);
    int y = scale(j->target_y, e) + scale(j->curve_y, arc);
    *dx = x - j->last_x;
    *dy = y - j->last_y;
    j->last_x = x;
    j->last_y = y;
    j->offset_x += *dx;
    j->offset_y += *dy;
}
