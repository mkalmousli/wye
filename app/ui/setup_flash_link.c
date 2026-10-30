/*
 * Setup without BOOTSEL: Picos that already run Wye are updated over USB
 * (the target through the bridge and the wires), and a Pico with old firmware
 * is restarted into install mode by software.
 */
#include "setup_flash.h"
#include "util/util.h"
#include <stdio.h>
#include <string.h>

#define RESCUE_WAIT_S 8.0
#define DONE_SHOW_S 2.5

static bool fw_busy(const struct wye_status *st)
{
    return st->fw_phase >= WYE_FWS_BEGIN && st->fw_phase <= WYE_FWS_REBOOT;
}

static bool reachable(const struct wye_status *st, uint8_t role)
{
    return role == WYE_ROLE_BRIDGE ? st->link.bridge.present : st->link.target.present;
}

static bool outdated(const struct wye_status *st, uint8_t role)
{
    return role == WYE_ROLE_BRIDGE ? st->bridge_outdated : st->target_outdated;
}

/* The update has ended: did the Pico come back with the new firmware? */
static void finished(struct wye_ui *ui, struct flash_state *s)
{
    const struct wye_status *st = &ui->st;
    if (reachable(st, s->role) && !outdated(st, s->role) && !st->fw_error[0]) {
        s->done[s->role] = true;
        flash_enter(s, FLASH_DONE);
        ui_confetti_burst((Vector2){(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 3});
    } else {
        snprintf(s->error, sizeof(s->error), "%s", st->fw_error[0] ? st->fw_error : "The update did not finish.");
        flash_enter(s, FLASH_FAILED);
    }
}

/* Target first: it is reached through the bridge, so update it while the bridge is known good. */
static bool start_next(struct wye_ui *ui, struct flash_state *s)
{
    static const uint8_t ORDER[] = {WYE_ROLE_TARGET, WYE_ROLE_BRIDGE};
    for (int i = 0; i < 2; ++i) {
        uint8_t r = ORDER[i];
        bool wanted = s->queued[r] || outdated(&ui->st, r);
        if (wanted && reachable(&ui->st, r) && wye_engine_update_firmware(r)) {
            s->queued[r] = false;
            s->role = r;
            flash_enter(s, FLASH_UPDATING);
            return true;
        }
    }
    return false;
}

bool flash_link_poll(struct wye_ui *ui, struct flash_state *s)
{
    const struct wye_status *st = &ui->st;
    if (fw_busy(st)) { /* ours, or an automatic update the engine started */
        s->role = st->fw_node == WYE_NODE_TARGET ? WYE_ROLE_TARGET : WYE_ROLE_BRIDGE;
        flash_enter(s, FLASH_UPDATING);
        return true;
    }
    if (s->phase == FLASH_UPDATING) {
        finished(ui, s);
        return true;
    }
    if (st->link.state == WYE_LINK_LEGACY && strcmp(s->rescued, st->link.legacy_port) != 0) {
        wye_strcpy(s->rescued, sizeof(s->rescued), st->link.legacy_port);
        if (wye_engine_rescue_legacy()) {
            flash_enter(s, FLASH_RESCUE); /* it reappears in BOOTSEL mode by itself */
            return true;
        }
    }
    if (s->phase == FLASH_RESCUE && GetTime() - s->since < RESCUE_WAIT_S) {
        return true;
    }
    bool showing_done = s->phase == FLASH_DONE && GetTime() - s->since < DONE_SHOW_S;
    return s->phase != FLASH_FAILED && !showing_done && start_next(ui, s);
}

/* "Install again": over USB where possible, BOOTSEL only for Picos it cannot reach. */
void flash_link_reinstall(struct wye_ui *ui, struct flash_state *s)
{
    s->reinstall = true;
    s->error[0] = '\0';
    for (uint8_t r = WYE_ROLE_BRIDGE; r <= WYE_ROLE_TARGET; ++r) {
        s->done[r] = false;
        s->queued[r] = reachable(&ui->st, r);
    }
    flash_enter(s, FLASH_WAITING);
}
