/* Consistent Overhead Byte Stuffing: removes 0x00 so it can delimit frames. */
#include "wye_codec.h"

size_t wye_cobs_encode(const uint8_t *in, size_t len, uint8_t *out, size_t cap)
{
    if (cap < len + len / 254u + 1u) {
        return 0;
    }
    size_t code_pos = 0;
    size_t o = 1;
    uint8_t code = 1;
    for (size_t i = 0; i < len; ++i) {
        if (in[i] != 0) {
            out[o++] = in[i];
            code++;
        }
        if (in[i] == 0 || code == 0xff) {
            out[code_pos] = code;
            code_pos = o++;
            code = 1;
        }
    }
    out[code_pos] = code;
    return o;
}

size_t wye_cobs_decode(const uint8_t *in, size_t len, uint8_t *out, size_t cap)
{
    size_t i = 0;
    size_t o = 0;
    while (i < len) {
        uint8_t code = in[i++];
        if (code == 0) {
            return WYE_COBS_ERROR;
        }
        for (uint8_t k = 1; k < code; ++k) {
            if (i >= len || in[i] == 0 || o >= cap) {
                return WYE_COBS_ERROR;
            }
            out[o++] = in[i++];
        }
        if (code != 0xff && i < len) {
            if (o >= cap) {
                return WYE_COBS_ERROR;
            }
            out[o++] = 0;
        }
    }
    return o;
}
