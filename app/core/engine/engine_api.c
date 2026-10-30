/* Thread-safe commands for the UI. Each takes the engine lock. */
#include "engine_internal.h"

#include "hid_usage.h"

#include <string.h>

#define LOCKED(body)                       \
    do {                                   \
        plat_mutex_lock(g_engine.lock);    \
        body;                              \
        plat_mutex_unlock(g_engine.lock);  \
        plat_event_signal(g_engine.wake);  \
    } while (0)

void wye_engine_apply_config(const struct wye_config *cfg)
{
    struct wye_engine *e = &g_engine;
    LOCKED({
        e->cfg = *cfg;
        e->input.hotkey = (uint16_t)cfg->hotkey;
        memcpy(e->input.chord_keys, cfg->shortcuts, sizeof(e->input.chord_keys));
        e->input.sensitivity = cfg->sensitivity;
        e->input.invert_wheel = cfg->invert_wheel;
        e->jiggler.interval_ms = cfg->jiggler_interval_s * 1000;
        plat_input_set_pointer_local(!cfg->use_mouse);
        wye_drive_configure(&e->drive, cfg->drive_enabled, cfg->drive_folder, (uint32_t)cfg->drive_size_mb);
    });
}

void wye_engine_set_mode(enum wye_fwd_mode mode)
{
    struct wye_engine *e = &g_engine;
    LOCKED({
        bool allowed = mode == WYE_FWD_OFF || wye_link_target_ready(&e->link);
        if (allowed) {
            wye_input_set_mode(&e->input, mode);
        }
    });
}

void wye_engine_toggle(void)
{
    LOCKED(wye_engine_do_toggle(&g_engine));
}

void wye_engine_type_text(const char *text)
{
    LOCKED({
        if (!wye_link_target_ready(&g_engine.link)) {
            wye_engine_notice_locked(&g_engine, "Cannot paste: target is not connected");
        } else if (wye_typing_text(&g_engine.typing, text) == 0) {
            wye_engine_notice_locked(&g_engine, "Clipboard has no typeable text");
        }
    });
}

void wye_engine_ctrl_alt_del(void)
{
    LOCKED(wye_typing_chord(&g_engine.typing, 0x05, HID_KEY_DELETE));
}

void wye_engine_jiggler(bool on)
{
    LOCKED(wye_jiggler_set(&g_engine.jiggler, on, plat_now_ms()));
}

void wye_engine_learn_key(bool start, bool for_shortcut)
{
    LOCKED({
        g_engine.learning_key = start;
        g_engine.learning_shortcut = for_shortcut;
        g_engine.learned_key = 0;
    });
}

unsigned wye_engine_take_ui_requests(void)
{
    unsigned r;
    plat_mutex_lock(g_engine.lock);
    r = g_engine.ui_requests;
    g_engine.ui_requests = 0;
    plat_mutex_unlock(g_engine.lock);
    return r;
}
