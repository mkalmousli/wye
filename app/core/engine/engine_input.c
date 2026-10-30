/* Capture-thread entry point and input routing callbacks. */
#include "engine_internal.h"

#include "hid_usage.h"
#include "log.h"

/* Every message for the target goes through here (input, jiggler, macros). */
static bool engine_send(void *ctx, uint8_t type, const void *payload, size_t len)
{
    struct wye_engine *e = ctx;
    if (!wye_link_target_ready(&e->link)) {
        return false;
    }
    wye_recorder_note(&e->recorder, plat_now_ms(), type, payload, len);
    e->activity.messages++;
    return wye_link_send(&e->link, WYE_NODE_TARGET, type, payload, len);
}

static void input_send(void *ctx, uint8_t type, const void *payload, size_t len)
{
    engine_send(ctx, type, payload, len);
}

static void input_action(void *ctx, enum wye_input_action action)
{
    struct wye_engine *e = ctx;
    switch (action) {
    case WYE_ACT_TOGGLE: wye_engine_do_toggle(e); break;
    case WYE_ACT_PASTE: e->ui_requests |= WYE_UI_REQ_PASTE; break;
    case WYE_ACT_SHOW_WINDOW: e->ui_requests |= WYE_UI_REQ_SHOW; break;
    case WYE_ACT_MINI: e->ui_requests |= WYE_UI_REQ_MINI; break;
    case WYE_ACT_FOLDER: e->ui_requests |= WYE_UI_REQ_FOLDER; break;
    case WYE_ACT_COUNT: break;
    case WYE_ACT_JIGGLER: wye_jiggler_set(&e->jiggler, !e->jiggler.active, plat_now_ms()); break;
    case WYE_ACT_CTRL_ALT_DEL:
        wye_typing_chord(&e->typing, 0x05 /* LCtrl+LAlt */, HID_KEY_DELETE);
        break;
    case WYE_ACT_RELEASE:
        wye_input_set_mode(&e->input, WYE_FWD_OFF);
        wye_typing_cancel(&e->typing);
        wye_engine_notice_locked(e, "Released all keys; control is back on this computer");
        break;
    }
}

static void input_mode_changed(void *ctx, enum wye_fwd_mode mode)
{
    (void)ctx;
    LOG_I("forwarding mode: %d", (int)mode);
    plat_timer_precision(mode != WYE_FWD_OFF);
    plat_input_set_forwarding(mode == WYE_FWD_RELATIVE);
}

void wye_engine_do_toggle(struct wye_engine *e)
{
    if (e->input.mode != WYE_FWD_OFF) {
        wye_input_set_mode(&e->input, WYE_FWD_OFF);
    } else if (!wye_link_target_ready(&e->link)) {
        wye_engine_notice_locked(e, "Target computer is not connected");
    } else {
        wye_input_set_mode(&e->input, WYE_FWD_RELATIVE);
    }
    plat_event_signal(e->wake);
}

void wye_engine_input_init(struct wye_engine *e)
{
    struct wye_input_out out = {e, input_send, input_action, input_mode_changed};
    wye_input_init(&e->input, &out);
    e->sender = (struct wye_sender){e, engine_send};
}
