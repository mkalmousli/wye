/* Input from the capture thread: hotkey learning, routing, and live activity. */
#include "engine_internal.h"

/* What the dashboard shows while controlling: keys pressed, pointer motion, buttons. */
void wye_engine_note_activity(struct wye_engine *e, const struct wye_input_event *ev)
{
    struct wye_activity *a = &e->activity;
    if (ev->type == WYE_EV_KEY && ev->down && !ev->repeat) {
        wye_activity_key(a, ev->code, plat_now_ms());
    } else if (ev->type == WYE_EV_MOVE) {
        a->dx += ev->dx;
        a->dy += ev->dy;
    } else if (ev->type == WYE_EV_WHEEL) {
        a->wheel += ev->dy / 120;
    }
    a->buttons = e->input.remote_buttons;
}

bool wye_engine_sink(void *ctx, const struct wye_input_event *ev)
{
    struct wye_engine *e = ctx;
    plat_mutex_lock(e->lock);
    if (e->learning_key && ev->type == WYE_EV_KEY) {
        if (ev->down && !ev->repeat) {
            e->learned_key = ev->code;
            e->learning_key = false;
        }
        plat_mutex_unlock(e->lock);
        return true;
    }
    bool shortcut = e->input.hotkey_held || ev->code == e->input.hotkey;
    bool pointer = ev->type == WYE_EV_BUTTON || ev->type == WYE_EV_MOVE || ev->type == WYE_EV_WHEEL;
    if (e->input.mode != WYE_FWD_OFF && !shortcut && !(pointer ? e->cfg.use_mouse : e->cfg.use_keyboard)) {
        plat_mutex_unlock(e->lock);
        return false; /* switched off in Devices: this one stays on this computer */
    }
    bool swallow = wye_input_event(&e->input, ev);
    if (e->input.mode != WYE_FWD_OFF && !shortcut) {
        wye_engine_note_activity(e, ev);
    }
    plat_mutex_unlock(e->lock);
    return swallow;
}
