/* The engine as the local API sees it: input for the target that bypasses forwarding. */
#include "engine_internal.h"

#include "api/api.h"
#include "product.h"
#include "wye_codec.h"

#include <stdio.h>

static enum wye_api_state api_state_locked(const struct wye_engine *e)
{
    if (!wye_link_target_ready(&e->link)) {
        return WYE_API_NOT_READY;
    }
    return e->input.mode != WYE_FWD_OFF || e->player.active ? WYE_API_BUSY : WYE_API_READY;
}

static enum wye_api_state api_state(void)
{
    plat_mutex_lock(g_engine.lock);
    enum wye_api_state s = api_state_locked(&g_engine);
    plat_mutex_unlock(g_engine.lock);
    return s;
}

static size_t api_type(const char *text)
{
    plat_mutex_lock(g_engine.lock);
    size_t n = wye_typing_text(&g_engine.typing, text);
    plat_mutex_unlock(g_engine.lock);
    plat_event_signal(g_engine.wake);
    return n;
}

static bool api_chord(uint8_t modifiers, uint8_t key)
{
    plat_mutex_lock(g_engine.lock);
    bool ok = wye_typing_chord(&g_engine.typing, modifiers, key);
    plat_mutex_unlock(g_engine.lock);
    plat_event_signal(g_engine.wake);
    return ok;
}

static bool api_typing_busy(void)
{
    plat_mutex_lock(g_engine.lock);
    bool busy = wye_typing_busy(&g_engine.typing);
    plat_mutex_unlock(g_engine.lock);
    return busy;
}

static bool api_mouse(uint8_t buttons, int dx, int dy, int wheel, int pan)
{
    struct wye_mouse m = {buttons, (int16_t)dx, (int16_t)dy, (int8_t)wheel, (int8_t)pan};
    uint8_t buf[WYE_MOUSE_LEN];
    plat_mutex_lock(g_engine.lock);
    bool ok = wye_send(&g_engine.sender, WYE_MSG_MOUSE, buf, wye_mouse_pack(&m, buf));
    plat_mutex_unlock(g_engine.lock);
    return ok;
}

static bool api_mouse_abs(uint8_t buttons, int x, int y)
{
    struct wye_abs_mouse m = {buttons, (uint16_t)x, (uint16_t)y, 0, 0};
    uint8_t buf[WYE_ABS_MOUSE_LEN];
    plat_mutex_lock(g_engine.lock);
    bool ok = (g_engine.link.info.target.hello.caps & WYE_CAP_ABS_MOUSE) &&
              wye_send(&g_engine.sender, WYE_MSG_ABS_MOUSE, buf, wye_abs_mouse_pack(&m, buf));
    plat_mutex_unlock(g_engine.lock);
    return ok;
}

static void api_release_all(void)
{
    plat_mutex_lock(g_engine.lock);
    wye_typing_cancel(&g_engine.typing);
    if (wye_link_target_ready(&g_engine.link)) {
        wye_send(&g_engine.sender, WYE_MSG_RELEASE_ALL, NULL, 0);
    }
    plat_mutex_unlock(g_engine.lock);
}

static void api_status(char *out, size_t len)
{
    plat_mutex_lock(g_engine.lock);
    static const char *const STATES[] = {"ready", "disconnected", "busy"};
    snprintf(out, len, "version=%s target=%s absolute=%d", WYE_VERSION_STRING, STATES[api_state_locked(&g_engine)],
             (g_engine.link.info.target.hello.caps & WYE_CAP_ABS_MOUSE) ? 1 : 0);
    plat_mutex_unlock(g_engine.lock);
}

const struct wye_api_ops *wye_engine_api_ops(void)
{
    static const struct wye_api_ops OPS = {api_state, api_type, api_chord, api_typing_busy, api_mouse,
                                           api_mouse_abs, api_release_all, api_status};
    return &OPS;
}
