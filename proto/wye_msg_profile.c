/* Pico profile payload: u8 colour, then the name and notes as length-prefixed strings. */
#include "wye_msg.h"

#include <string.h>

static size_t put(uint8_t *out, const char *s, size_t max)
{
    size_t n = 0;
    while (n < max && s[n]) {
        n++;
    }
    out[0] = (uint8_t)n;
    memcpy(out + 1, s, n);
    return n + 1;
}

static bool get(const uint8_t *p, size_t len, size_t *pos, char *dst, size_t max)
{
    if (*pos >= len || p[*pos] > max || *pos + 1 + p[*pos] > len) {
        return false;
    }
    size_t n = p[*pos];
    memcpy(dst, p + *pos + 1, n);
    dst[n] = '\0';
    *pos += 1 + n;
    return true;
}

size_t wye_profile_pack(const struct wye_profile *f, uint8_t *out)
{
    out[0] = f->color;
    size_t n = 1;
    n += put(out + n, f->name, WYE_PROFILE_NAME_MAX);
    n += put(out + n, f->notes, WYE_PROFILE_NOTES_MAX);
    return n;
}

bool wye_profile_unpack(const uint8_t *p, size_t len, struct wye_profile *f)
{
    size_t pos = 1;
    if (len < 1) {
        return false;
    }
    memset(f, 0, sizeof(*f));
    f->color = p[0] < WYE_PROFILE_COLORS ? p[0] : 0;
    return get(p, len, &pos, f->name, WYE_PROFILE_NAME_MAX) && get(p, len, &pos, f->notes, WYE_PROFILE_NOTES_MAX);
}

void wye_profile_clean(char *text, size_t max)
{
    size_t len = 0;
    for (const char *s = text; *s; ++s) {
        if (len || ((unsigned char)*s > ' ' && *s != 0x7f)) { /* leading spaces go */
            text[len++] = (unsigned char)*s < ' ' || *s == 0x7f ? ' ' : *s;
        }
    }
    text[len] = '\0';
    if (len > max) {
        len = max;
        while (len > 0 && ((unsigned char)text[len] & 0xc0) == 0x80) {
            len--; /* never cut a character in half */
        }
    }
    while (len > 0 && text[len - 1] == ' ') {
        len--;
    }
    text[len] = '\0';
}
