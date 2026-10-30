#include "fw_session.h"

#include "log.h"
#include "util/util.h"

#include <stdlib.h>

static void fail(struct wye_fw_session *s, const char *why)
{
    LOG_E("firmware: %s update failed: %s", s->image ? s->image->name : "?", why);
    wye_strcpy(s->error, sizeof(s->error), why);
    free(s->bin);
    s->bin = NULL;
    s->phase = WYE_FWS_FAILED;
}

static void enter(struct wye_fw_session *s, enum wye_fw_phase phase, uint64_t now)
{
    s->phase = phase;
    s->phase_since = now;
    s->sent_at = 0; /* send immediately on next tick */
    s->retries = 0;
}

bool wye_fw_session_start(struct wye_fw_session *s, uint8_t node, const struct wye_fw_image *img,
                          uint64_t now)
{
    wye_fw_session_reset(s);
    s->node = node;
    s->image = img;
    if (!img || !wye_uf2_to_bin(img->uf2, img->uf2_len, &s->bin, &s->len)) {
        fail(s, "embedded firmware image is missing or invalid");
        return false;
    }
    s->crc = wye_crc32(s->bin, s->len);
    LOG_I("firmware: updating %s to %u.%u.%u (%lu bytes)", img->name, img->major, img->minor,
          img->patch, (unsigned long)s->len);
    enter(s, WYE_FWS_BEGIN, now);
    return true;
}

void wye_fw_session_packet(struct wye_fw_session *s, const struct wye_packet *pkt, uint64_t now)
{
    if (pkt->node != s->node || pkt->type != WYE_MSG_FW_ACK || pkt->len < 5) {
        return;
    }
    uint8_t status = pkt->payload[0];
    uint32_t next = wye_get_u32(pkt->payload + 1);
    if (status == WYE_FW_ERR_OFFSET && s->phase == WYE_FWS_DATA) {
        s->offset = next; /* resync to what the device has */
        s->sent_at = 0;
        return;
    }
    if (status != WYE_FW_OK) {
        static const char *why[] = {"ok", "device state", "image too large", "offset", "checksum",
                                    "image rejected by device", "flash write"};
        fail(s, status < WYE_ARRAY_LEN(why) ? why[status] : "device error");
        return;
    }
    if (s->phase == WYE_FWS_BEGIN) {
        s->offset = 0;
        enter(s, WYE_FWS_DATA, now);
    } else if (s->phase == WYE_FWS_DATA) {
        s->offset = next;
        s->retries = 0;
        s->sent_at = 0;
        if (s->offset >= s->len) {
            enter(s, WYE_FWS_COMMIT, now);
        }
    } else if (s->phase == WYE_FWS_COMMIT) {
        free(s->bin);
        s->bin = NULL;
        enter(s, WYE_FWS_REBOOT, now);
    }
}

bool wye_fw_session_busy(const struct wye_fw_session *s)
{
    return s->phase >= WYE_FWS_BEGIN && s->phase <= WYE_FWS_REBOOT;
}

int wye_fw_session_progress(const struct wye_fw_session *s)
{
    if (s->phase == WYE_FWS_DATA && s->len) {
        return (int)(5 + (uint64_t)s->offset * 85 / s->len);
    }
    static const int pct[] = {0, 2, 0, 92, 95, 100, 0};
    return pct[s->phase];
}

void wye_fw_session_reset(struct wye_fw_session *s)
{
    free(s->bin);
    *s = (struct wye_fw_session){0};
}
