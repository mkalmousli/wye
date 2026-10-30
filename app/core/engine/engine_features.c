/* Periodic feature work and reactions to link events (engine lock held). */
#include "engine_internal.h"

#include "log.h"

#include <stdio.h>

static void maybe_auto_update(struct wye_engine *e, uint64_t now)
{
    if (!e->cfg.auto_firmware || wye_fw_session_busy(&e->fw) || e->input.mode != WYE_FWD_OFF ||
        e->player.active) {
        return;
    }
    const struct wye_node_info *nodes[] = {&e->link.info.target, &e->link.info.bridge};
    const uint8_t ids[] = {WYE_NODE_TARGET, WYE_NODE_BRIDGE};
    for (int i = 0; i < 2; ++i) {
        uint8_t role = ids[i] == WYE_NODE_TARGET ? WYE_ROLE_TARGET : WYE_ROLE_BRIDGE;
        const struct wye_fw_image *img = wye_fw_embedded(role);
        const struct wye_hello_ack *h = &nodes[i]->hello;
        if (nodes[i]->present && img && !e->auto_fw_tried[ids[i]] &&
            (h->caps & WYE_CAP_FW_UPDATE) && wye_fw_is_older(img, h->fw_major, h->fw_minor, h->fw_patch)) {
            e->auto_fw_tried[ids[i]] = true;
            wye_engine_notice_locked(e, "Updating device firmware - keep it plugged in");
            wye_fw_session_start(&e->fw, ids[i], img, now);
            return; /* one node at a time: target first */
        }
    }
}

void wye_engine_tick(struct wye_engine *e, uint64_t now)
{
    e->jiggler.paused = e->player.active;
    wye_jiggler_tick(&e->jiggler, now, &e->sender);
    wye_typing_tick(&e->typing, now, &e->sender);
    wye_player_tick(&e->player, now, &e->sender);
    wye_fw_session_tick(&e->fw, &e->link, now);
    if (e->fw.phase == WYE_FWS_DONE || e->fw.phase == WYE_FWS_FAILED) {
        wye_engine_notice_locked(e, e->fw.phase == WYE_FWS_DONE ? "Firmware updated" : e->fw.error);
        e->fw.phase = WYE_FWS_IDLE;
    }
    maybe_auto_update(e, now);
    const struct wye_node_info *t = &e->link.info.target;
    wye_drive_tick(&e->drive, now, wye_link_target_ready(&e->link), t->present && (t->hello.caps & WYE_CAP_DISK),
                   &e->sender);
    bool sound = wye_link_target_ready(&e->link) && (t->hello.caps & WYE_CAP_AUDIO) &&
                 (e->link.info.bridge.hello.caps & WYE_CAP_AUDIO);
    wye_audio_want(sound ? e->link.info.product : "", e->cfg.use_speaker, e->cfg.use_mic);
}

void wye_engine_on_packet(void *ctx, const struct wye_packet *pkt)
{
    struct wye_engine *e = ctx;
    wye_fw_session_packet(&e->fw, pkt, plat_now_ms());
    if (pkt->node == WYE_NODE_TARGET && pkt->type >= WYE_MSG_DISK_SET && pkt->type <= WYE_MSG_DISK_WACK) {
        wye_drive_packet(&e->drive, pkt, &e->sender);
    }
    if (pkt->type == WYE_MSG_FUNCS && pkt->node == WYE_NODE_TARGET) {
        e->functions_valid = wye_functions_unpack(pkt->payload, pkt->len, &e->functions);
    }
    if (pkt->type == WYE_MSG_IDENTITY && pkt->node <= WYE_NODE_TARGET) {
        e->identity_valid[pkt->node] = wye_identity_unpack(pkt->payload, pkt->len, &e->identity[pkt->node]);
    }
    if (pkt->type == WYE_MSG_LOG && pkt->len && pkt->node <= WYE_NODE_TARGET) { /* e.g. "restarted after..." */
        char msg[WYE_NOTICE_LEN];
        snprintf(msg, sizeof(msg), "%s Pico %.*s", pkt->node == WYE_NODE_TARGET ? "Target" : "Bridge",
                 (int)pkt->len, (const char *)pkt->payload);
        wye_engine_notice_locked(e, msg);
    }
    if (pkt->node == WYE_NODE_TARGET && (pkt->type == WYE_MSG_LEDS || pkt->type == WYE_MSG_PONG)) {
        plat_input_set_leds(e->link.info.leds);
    }
}

void wye_engine_on_state(void *ctx, enum wye_link_state old_state, enum wye_link_state new_state)
{
    struct wye_engine *e = ctx;
    if (old_state == WYE_LINK_READY && new_state != WYE_LINK_READY) {
        if (e->input.mode != WYE_FWD_OFF) {
            wye_input_set_mode(&e->input, WYE_FWD_OFF);
            wye_engine_notice_locked(e, "Target link lost - control returned to this computer");
        }
        wye_typing_cancel(&e->typing);
        if (e->player.active && !wye_fw_session_busy(&e->fw)) {
            wye_player_stop(&e->player);
            wye_engine_notice_locked(e, "Routine stopped: target disconnected");
        }
    }
    if (new_state == WYE_LINK_SEARCHING || new_state == WYE_LINK_LEGACY) {
        if (!wye_fw_session_busy(&e->fw)) {
            e->auto_fw_tried[WYE_NODE_BRIDGE] = e->auto_fw_tried[WYE_NODE_TARGET] = false;
        }
    }
    if (new_state == WYE_LINK_READY) {
        wye_engine_notice_locked(e, "Target computer connected");
    } else if (new_state == WYE_LINK_LEGACY) {
        wye_engine_notice_locked(e, "Found a Pico with old firmware - open Setup to update it");
    }
}
