/* Session payloads: HELLO_ACK and PONG. */
#include "wye_msg.h"

#include <string.h>

size_t wye_hello_ack_pack(const struct wye_hello_ack *a, uint8_t *out)
{
    out[0] = a->proto_version;
    out[1] = a->role;
    out[2] = a->fw_major;
    out[3] = a->fw_minor;
    out[4] = a->fw_patch;
    out[5] = a->board;
    wye_put_u16(&out[6], a->caps);
    memcpy(&out[8], a->uid, 8);
    wye_put_u32(&out[16], a->build_id);
    return WYE_HELLO_ACK_LEN;
}

bool wye_hello_ack_unpack(const uint8_t *p, size_t len, struct wye_hello_ack *a)
{
    if (len < WYE_HELLO_ACK_LEN) {
        return false;
    }
    a->proto_version = p[0];
    a->role = p[1];
    a->fw_major = p[2];
    a->fw_minor = p[3];
    a->fw_patch = p[4];
    a->board = p[5];
    a->caps = wye_get_u16(&p[6]);
    memcpy(a->uid, &p[8], 8);
    a->build_id = wye_get_u32(&p[16]);
    return true;
}

size_t wye_pong_pack(const struct wye_pong *a, uint8_t *out)
{
    wye_put_u32(&out[0], a->token);
    out[4] = a->status;
    out[5] = a->leds;
    wye_put_u16(&out[6], a->rx_errors);
    wye_put_u32(&out[8], a->uptime_ms);
    return WYE_PONG_LEN;
}

bool wye_pong_unpack(const uint8_t *p, size_t len, struct wye_pong *a)
{
    if (len < WYE_PONG_LEN) {
        return false;
    }
    a->token = wye_get_u32(&p[0]);
    a->status = p[4];
    a->leds = p[5];
    a->rx_errors = wye_get_u16(&p[6]);
    a->uptime_ms = wye_get_u32(&p[8]);
    return true;
}
