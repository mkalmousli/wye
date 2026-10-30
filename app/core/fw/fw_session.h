/*
 * Host side of a firmware update over the link (stop-and-wait with
 * retries). Finishes only when the node is back with the new version.
 */
#ifndef WYE_FW_SESSION_H
#define WYE_FW_SESSION_H

#include "fw_image.h"
#include "link/link.h"

enum wye_fw_phase {
    WYE_FWS_IDLE = 0,
    WYE_FWS_BEGIN,
    WYE_FWS_DATA,
    WYE_FWS_COMMIT,
    WYE_FWS_REBOOT,
    WYE_FWS_DONE,
    WYE_FWS_FAILED,
};

#define WYE_FWS_ACK_TIMEOUT_MS 500u
#define WYE_FWS_RETRIES 6
#define WYE_FWS_REBOOT_TIMEOUT_MS 20000u

struct wye_fw_session {
    enum wye_fw_phase phase;
    uint8_t node;                 /* WYE_NODE_BRIDGE / WYE_NODE_TARGET */
    const struct wye_fw_image *image;
    uint8_t *bin;
    size_t len;
    uint32_t crc;
    uint32_t offset;              /* next byte the device wants */
    uint64_t sent_at, phase_since;
    int retries;
    bool node_left;               /* went away after commit (rebooting) */
    char error[128];
};

bool wye_fw_session_start(struct wye_fw_session *s, uint8_t node, const struct wye_fw_image *img,
                          uint64_t now);
void wye_fw_session_packet(struct wye_fw_session *s, const struct wye_packet *pkt, uint64_t now);
void wye_fw_session_tick(struct wye_fw_session *s, struct wye_link *link, uint64_t now);
bool wye_fw_session_busy(const struct wye_fw_session *s);
int wye_fw_session_progress(const struct wye_fw_session *s);
void wye_fw_session_reset(struct wye_fw_session *s);

#endif
