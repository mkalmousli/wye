#include "wye_codec.h"

#include <string.h>

void wye_decoder_init(struct wye_decoder *d)
{
    memset(d, 0, sizeof(*d));
}

static enum wye_decode finish_frame(struct wye_decoder *d, struct wye_packet *pkt)
{
    if (d->overflow) {
        return WYE_DECODE_ERROR;
    }
    if (d->raw_len == 0) {
        return WYE_DECODE_NONE; /* back-to-back delimiters are harmless */
    }
    uint8_t decoded[WYE_MAX_PACKET];
    size_t n = wye_cobs_decode(d->raw, d->raw_len, decoded, sizeof(decoded));
    if (n == WYE_COBS_ERROR || !wye_packet_parse(decoded, n, pkt)) {
        return WYE_DECODE_ERROR;
    }
    return WYE_DECODE_PACKET;
}

enum wye_decode wye_decoder_feed(struct wye_decoder *d, uint8_t byte, struct wye_packet *pkt)
{
    if (d->has_frame) {
        /* Previous call delivered a packet; its raw bytes are no longer needed. */
        d->has_frame = false;
        d->raw_len = 0;
    }
    if (byte != 0) {
        if (d->raw_len < sizeof(d->raw)) {
            d->raw[d->raw_len++] = byte;
        } else {
            d->overflow = true;
        }
        return WYE_DECODE_NONE;
    }
    enum wye_decode r = finish_frame(d, pkt);
    d->overflow = false;
    if (r == WYE_DECODE_PACKET) {
        d->has_frame = true;
    } else {
        d->raw_len = 0;
        if (r == WYE_DECODE_ERROR) {
            d->errors++;
        }
    }
    return r;
}
