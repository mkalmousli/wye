/* Firmware session: (re)transmission and reboot tracking. */
#include "fw_session.h"

#include "log.h"
#include "util/util.h"

#include <string.h>

static void send_current(struct wye_fw_session *s, struct wye_link *link)
{
    uint8_t p[4 + WYE_FW_CHUNK];
    if (s->phase == WYE_FWS_BEGIN) {
        wye_put_u32(p, (uint32_t)s->len);
        wye_put_u32(p + 4, s->crc);
        wye_link_send(link, s->node, WYE_MSG_FW_BEGIN, p, 8);
    } else if (s->phase == WYE_FWS_DATA) {
        size_t n = s->len - s->offset < WYE_FW_CHUNK ? s->len - s->offset : WYE_FW_CHUNK;
        wye_put_u32(p, s->offset);
        memcpy(p + 4, s->bin + s->offset, n);
        wye_link_send(link, s->node, WYE_MSG_FW_DATA, p, 4 + n);
    } else if (s->phase == WYE_FWS_COMMIT) {
        wye_link_send(link, s->node, WYE_MSG_FW_COMMIT, NULL, 0);
    }
}

static void track_reboot(struct wye_fw_session *s, const struct wye_link *link, uint64_t now)
{
    const struct wye_node_info *n = s->node == WYE_NODE_TARGET ? &link->info.target : &link->info.bridge;
    if (!n->present) {
        s->node_left = true;
    } else if (s->node_left) {
        const struct wye_hello_ack *h = &n->hello;
        if (wye_fw_is_older(s->image, h->fw_major, h->fw_minor, h->fw_patch)) {
            wye_strcpy(s->error, sizeof(s->error), "device restarted with the old firmware");
            s->phase = WYE_FWS_FAILED;
        } else {
            LOG_I("firmware: %s now runs %u.%u.%u", s->image->name, h->fw_major, h->fw_minor, h->fw_patch);
            s->phase = WYE_FWS_DONE;
        }
        return;
    }
    if (now - s->phase_since > WYE_FWS_REBOOT_TIMEOUT_MS) {
        wye_strcpy(s->error, sizeof(s->error), "device did not come back after the update");
        s->phase = WYE_FWS_FAILED;
    }
}

void wye_fw_session_tick(struct wye_fw_session *s, struct wye_link *link, uint64_t now)
{
    if (s->phase == WYE_FWS_REBOOT) {
        track_reboot(s, link, now);
        return;
    }
    if (!wye_fw_session_busy(s)) {
        return;
    }
    bool node_up = s->node == WYE_NODE_TARGET ? link->info.target.present : link->info.bridge.present;
    if (!node_up) {
        if (now - s->phase_since > WYE_FWS_REBOOT_TIMEOUT_MS) {
            wye_strcpy(s->error, sizeof(s->error), "device disconnected during the update");
            s->phase = WYE_FWS_FAILED;
        }
        return; /* wait: the link may come back */
    }
    if (s->sent_at && now - s->sent_at < WYE_FWS_ACK_TIMEOUT_MS) {
        return;
    }
    if (s->sent_at && ++s->retries > WYE_FWS_RETRIES) {
        wye_strcpy(s->error, sizeof(s->error), "device stopped acknowledging");
        s->phase = WYE_FWS_FAILED;
        return;
    }
    send_current(s, link);
    s->sent_at = now;
}
