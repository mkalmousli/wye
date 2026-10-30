/*
 * Mouse jiggler: every interval, moves the target pointer along a smooth,
 * slightly curved path, pauses, then returns to where it started.
 */
#ifndef WYE_JIGGLER_H
#define WYE_JIGGLER_H

#include "sender.h"

enum wye_jiggle_phase { WYE_JIG_IDLE = 0, WYE_JIG_OUT, WYE_JIG_DWELL, WYE_JIG_BACK };

struct wye_jiggler {
    bool active;
    bool paused;                /* e.g. while a routine plays */
    int interval_ms;
    enum wye_jiggle_phase phase;
    uint64_t next_at;
    uint64_t leg_start;
    int leg_ms;
    int target_x, target_y, curve_x, curve_y;
    int last_x, last_y;         /* position along the current leg */
    int offset_x, offset_y;     /* total displacement from origin */
    unsigned long moves;
    uint32_t rng;
};

void wye_jiggler_set(struct wye_jiggler *j, bool active, uint64_t now);
void wye_jiggler_tick(struct wye_jiggler *j, uint64_t now, const struct wye_sender *out);
/* Progress through the current cycle in percent, for the UI. */
int wye_jiggler_progress(const struct wye_jiggler *j, uint64_t now);

/* Internal (jiggler_path.c). */
int wye_jiggler_rand(struct wye_jiggler *j, int lo, int hi);
void wye_jiggler_begin_leg(struct wye_jiggler *j, uint64_t now, enum wye_jiggle_phase phase,
                           int tx, int ty);
/* Advances along the leg; returns the delta to move by. */
void wye_jiggler_step(struct wye_jiggler *j, uint64_t now, int *dx, int *dy);

#endif
