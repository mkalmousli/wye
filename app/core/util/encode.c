/* Hex encoder (hashes, signatures, settings). */
#include "util.h"

#include <ctype.h>
#include <string.h>

static const char HEX[] = "0123456789abcdef";

size_t wye_hex_encode(const uint8_t *in, size_t len, char *out, size_t cap)
{
    if (cap < len * 2 + 1) {
        return 0;
    }
    for (size_t i = 0; i < len; ++i) {
        out[i * 2] = HEX[in[i] >> 4];
        out[i * 2 + 1] = HEX[in[i] & 15];
    }
    out[len * 2] = '\0';
    return len * 2;
}

static int hex_val(char c)
{
    const char *p = c ? strchr(HEX, tolower((unsigned char)c)) : NULL;
    return p ? (int)(p - HEX) : -1;
}

size_t wye_hex_decode(const char *in, uint8_t *out, size_t cap)
{
    size_t n = strlen(in);
    if (n % 2 || n / 2 > cap) {
        return (size_t)-1;
    }
    for (size_t i = 0; i < n / 2; ++i) {
        int hi = hex_val(in[i * 2]);
        int lo = hex_val(in[i * 2 + 1]);
        if (hi < 0 || lo < 0) {
            return (size_t)-1;
        }
        out[i] = (uint8_t)(hi << 4 | lo);
    }
    return n / 2;
}
