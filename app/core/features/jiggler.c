#include "jiggler.h"

#include "wye_msg.h"
#include "wye_proto.h"

#define STEP_MS 16

void wye_jiggler_set(struct wye_jiggler *j, bool active, uint64_t now)
{
    if (j->active == active) {
        return;
    }
    int interval = j->interval_ms > 0 ? j->interval_ms : 60000;
    uint32_t rng = j->rng ? j->rng : (uint32_t)now | 1u;
    *j = (struct wye_jiggler){0};
    j->active = active;
    j->interval_ms = interval;
    j->rng = rng;
    j->next_at = now + 1000;
}

static void send_move(const struct wye_sender *out, int dx, int dy)
{
    struct wye_mouse m = {0, (int16_t)dx, (int16_t)dy, 0, 0};
    uint8_t buf[WYE_MOUSE_LEN];
    wye_send(out, WYE_MSG_MOUSE, buf, wye_mouse_pack(&m, buf));
}

static void start_outbound(struct wye_jiggler *j, uint64_t now)
{
    int tx = wye_jiggler_rand(j, 140, 360) * (wye_jiggler_rand(j, 0, 1) ? 1 : -1);
    int ty = wye_jiggler_rand(j, 25, 130) * (wye_jiggler_rand(j, 0, 1) ? 1 : -1);
    wye_jiggler_begin_leg(j, now, WYE_JIG_OUT, tx, ty);
}

void wye_jiggler_tick(struct wye_jiggler *j, uint64_t now, const struct wye_sender *out)
{
    if (!j->active || j->paused || now < j->next_at) {
        return;
    }
    if (j->phase == WYE_JIG_IDLE) {
        start_outbound(j, now);
    } else if (j->phase == WYE_JIG_DWELL) {
        wye_jiggler_begin_leg(j, now, WYE_JIG_BACK, -j->offset_x, -j->offset_y);
    }
    int dx, dy;
    wye_jiggler_step(j, now, &dx, &dy);
    if (dx || dy) {
        send_move(out, dx, dy);
        j->moves++;
    }
    j->next_at = now + STEP_MS;
    if (now - j->leg_start < (uint64_t)j->leg_ms) {
        return;
    }
    if (j->phase == WYE_JIG_OUT) {
        j->phase = WYE_JIG_DWELL;
        j->next_at = now + (uint64_t)wye_jiggler_rand(j, 450, 1800);
    } else {
        j->phase = WYE_JIG_IDLE;
        j->next_at = now + (uint64_t)wye_jiggler_rand(j, j->interval_ms / 2, j->interval_ms * 3 / 2);
    }
}

int wye_jiggler_progress(const struct wye_jiggler *j, uint64_t now)
{
    if (!j->active || j->paused) {
        return 0;
    }
    if (j->phase == WYE_JIG_IDLE && j->next_at > now) {
        uint64_t left = j->next_at - now;
        uint64_t span = (uint64_t)j->interval_ms;
        return left >= span ? 0 : (int)((span - left) * 100 / span);
    }
    return 100;
}
