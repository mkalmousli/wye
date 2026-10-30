/* Device maintenance commands: firmware updates, reboots, legacy rescue. */
#include "engine_internal.h"

#include "log.h"
#include "util/util.h"

bool wye_engine_update_firmware(uint8_t role)
{
    struct wye_engine *e = &g_engine;
    bool ok = false;
    uint8_t node = role == WYE_ROLE_TARGET ? WYE_NODE_TARGET : WYE_NODE_BRIDGE;
    plat_mutex_lock(e->lock);
    if (!wye_fw_session_busy(&e->fw) && e->input.mode == WYE_FWD_OFF) {
        ok = wye_fw_session_start(&e->fw, node, wye_fw_embedded(role), plat_now_ms());
    }
    plat_mutex_unlock(e->lock);
    plat_event_signal(e->wake);
    return ok;
}

bool wye_engine_reboot_node(uint8_t node, uint8_t mode)
{
    bool ok = false;
    plat_mutex_lock(g_engine.lock);
    ok = wye_link_send(&g_engine.link, node, WYE_MSG_REBOOT, &mode, 1);
    plat_mutex_unlock(g_engine.lock);
    return ok;
}

bool wye_engine_identity_request(uint8_t node)
{
    plat_mutex_lock(g_engine.lock);
    bool ok = wye_link_send(&g_engine.link, node, WYE_MSG_IDENTITY_GET, NULL, 0);
    plat_mutex_unlock(g_engine.lock);
    return ok;
}

bool wye_engine_identity_set(uint8_t node, const struct wye_identity *id)
{
    uint8_t buf[4 + 3 * WYE_ID_STR_MAX];
    size_t n = wye_identity_pack(id, buf);
    plat_mutex_lock(g_engine.lock);
    bool ok = wye_link_send(&g_engine.link, node, WYE_MSG_IDENTITY_SET, buf, n);
    plat_mutex_unlock(g_engine.lock);
    LOG_I("identity: %04x:%04x '%s' sent to node %u", id->vid, id->pid, id->product, node);
    return ok;
}

bool wye_engine_functions_request(void)
{
    plat_mutex_lock(g_engine.lock);
    bool ok = wye_link_send(&g_engine.link, WYE_NODE_TARGET, WYE_MSG_FUNCS_GET, NULL, 0);
    plat_mutex_unlock(g_engine.lock);
    return ok;
}

bool wye_engine_functions_set(const struct wye_functions *f)
{
    uint8_t buf[1 + 2 * WYE_FN_NAME_MAX];
    size_t n = wye_functions_pack(f, buf);
    plat_mutex_lock(g_engine.lock);
    bool ok = wye_link_send(&g_engine.link, WYE_NODE_TARGET, WYE_MSG_FUNCS_SET, buf, n);
    g_engine.functions_valid = false; /* re-read after the restart */
    plat_mutex_unlock(g_engine.lock);
    return ok;
}

bool wye_engine_profile_set(uint8_t node, const struct wye_profile *f)
{
    uint8_t buf[WYE_PROFILE_PACKED_MAX];
    size_t n = wye_profile_pack(f, buf);
    plat_mutex_lock(g_engine.lock);
    bool ok = wye_link_send(&g_engine.link, node, WYE_MSG_PROFILE_SET, buf, n);
    plat_mutex_unlock(g_engine.lock);
    return ok;
}

bool wye_engine_rescue_legacy(void)
{
    char port[PLAT_PORT_PATH_MAX];
    plat_mutex_lock(g_engine.lock);
    wye_strcpy(port, sizeof(port), g_engine.link.info.legacy_port);
    plat_mutex_unlock(g_engine.lock);
    if (!port[0]) {
        return false;
    }
    LOG_I("rescue: asking %s to enter BOOTSEL", port);
    return plat_serial_touch_1200(port);
}

void wye_engine_drive_sync_now(void)
{
    plat_mutex_lock(g_engine.lock);
    wye_drive_sync_now(&g_engine.drive);
    plat_mutex_unlock(g_engine.lock);
    plat_event_signal(g_engine.wake);
}
