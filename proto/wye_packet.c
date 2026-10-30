#include "wye_codec.h"
#include "wye_msg.h"

#include <string.h>

size_t wye_packet_encode(uint8_t node, uint8_t type, uint8_t seq, const void *payload, size_t len,
                         uint8_t *out, size_t cap)
{
    uint8_t raw[WYE_MAX_PACKET];
    if (len > WYE_MAX_PAYLOAD) {
        return 0;
    }
    raw[0] = node;
    raw[1] = type;
    raw[2] = seq;
    if (len) {
        memcpy(&raw[WYE_HEADER_LEN], payload, len);
    }
    wye_put_u16(&raw[WYE_HEADER_LEN + len], wye_crc16(raw, WYE_HEADER_LEN + len));
    size_t n = wye_cobs_encode(raw, WYE_HEADER_LEN + len + WYE_CRC_LEN, out, cap);
    if (n == 0 || n + 1 > cap) {
        return 0;
    }
    out[n++] = 0;
    return n;
}

bool wye_packet_parse(const uint8_t *decoded, size_t len, struct wye_packet *pkt)
{
    if (len < WYE_HEADER_LEN + WYE_CRC_LEN || len > WYE_MAX_PACKET) {
        return false;
    }
    size_t body = len - WYE_CRC_LEN;
    if (wye_crc16(decoded, body) != wye_get_u16(&decoded[body])) {
        return false;
    }
    pkt->node = decoded[0];
    pkt->type = decoded[1];
    pkt->seq = decoded[2];
    pkt->len = (uint8_t)(body - WYE_HEADER_LEN);
    memcpy(pkt->payload, &decoded[WYE_HEADER_LEN], pkt->len);
    return true;
}
