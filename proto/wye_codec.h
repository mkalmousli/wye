/* Framing: checksums, COBS, packet encode/parse and a streaming decoder. */
#ifndef WYE_CODEC_H
#define WYE_CODEC_H

#include "wye_proto.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint16_t wye_crc16(const uint8_t *data, size_t len);
uint32_t wye_crc32_update(uint32_t crc, const uint8_t *data, size_t len); /* start at 0 */
uint32_t wye_crc32(const uint8_t *data, size_t len);

/* Returns encoded length (no delimiter) or 0 if out is too small. */
size_t wye_cobs_encode(const uint8_t *in, size_t len, uint8_t *out, size_t cap);
/* Returns decoded length or WYE_COBS_ERROR. */
#define WYE_COBS_ERROR ((size_t)-1)
size_t wye_cobs_decode(const uint8_t *in, size_t len, uint8_t *out, size_t cap);

struct wye_packet {
    uint8_t node;
    uint8_t type;
    uint8_t seq;
    uint8_t len;
    uint8_t payload[WYE_MAX_PAYLOAD];
};

/* Encodes header+payload+crc, COBS and the trailing 0x00. Returns 0 on error. */
size_t wye_packet_encode(uint8_t node, uint8_t type, uint8_t seq, const void *payload, size_t len,
                         uint8_t *out, size_t cap);
bool wye_packet_parse(const uint8_t *decoded, size_t len, struct wye_packet *pkt);

enum wye_decode { WYE_DECODE_NONE = 0, WYE_DECODE_PACKET, WYE_DECODE_ERROR };

/*
 * Streaming decoder. After WYE_DECODE_PACKET, raw[0..raw_len) still holds
 * the encoded frame (without delimiter) so relays can forward it verbatim.
 */
struct wye_decoder {
    uint8_t raw[WYE_MAX_ENCODED];
    size_t raw_len;
    bool overflow;
    bool has_frame;
    uint32_t errors;
};

void wye_decoder_init(struct wye_decoder *d);
enum wye_decode wye_decoder_feed(struct wye_decoder *d, uint8_t byte, struct wye_packet *pkt);

#endif
