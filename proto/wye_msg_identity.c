/* USB identity payload: u16 vid, u16 pid, then three length-prefixed strings. */
#include "wye_msg.h"

#include <string.h>

static size_t put_str(uint8_t *out, const char *s)
{
    size_t n = 0;
    while (n < WYE_ID_STR_MAX - 1 && s[n]) {
        n++;
    }
    out[0] = (uint8_t)n;
    memcpy(out + 1, s, n);
    return n + 1;
}

static bool get_str(const uint8_t *p, size_t len, size_t *pos, char *dst)
{
    if (*pos >= len) {
        return false;
    }
    size_t n = p[*pos];
    if (n >= WYE_ID_STR_MAX || *pos + 1 + n > len) {
        return false;
    }
    memcpy(dst, p + *pos + 1, n);
    dst[n] = '\0';
    *pos += 1 + n;
    return true;
}

size_t wye_identity_pack(const struct wye_identity *id, uint8_t *out)
{
    wye_put_u16(&out[0], id->vid);
    wye_put_u16(&out[2], id->pid);
    size_t n = 4;
    n += put_str(out + n, id->manufacturer);
    n += put_str(out + n, id->product);
    n += put_str(out + n, id->serial);
    return n;
}

bool wye_identity_unpack(const uint8_t *p, size_t len, struct wye_identity *id)
{
    if (len < 4) {
        return false;
    }
    id->vid = wye_get_u16(&p[0]);
    id->pid = wye_get_u16(&p[2]);
    size_t pos = 4;
    return get_str(p, len, &pos, id->manufacturer) && get_str(p, len, &pos, id->product) &&
           get_str(p, len, &pos, id->serial);
}
