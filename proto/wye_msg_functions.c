/* Target settings payload: u8 flags, then the drive vendor and product as length-prefixed strings. */
#include "wye_msg.h"
#include <string.h>

static size_t put(uint8_t *out, const char *s, size_t max)
{
    size_t n = 0;
    while (n < max - 1 && s[n]) {
        n++;
    }
    out[0] = (uint8_t)n;
    memcpy(out + 1, s, n);
    return n + 1;
}

static bool get(const uint8_t *p, size_t len, size_t *pos, char *dst, size_t max)
{
    if (*pos >= len || p[*pos] >= max || *pos + 1 + p[*pos] > len) {
        return false;
    }
    size_t n = p[*pos];
    memcpy(dst, p + *pos + 1, n);
    dst[n] = '\0';
    *pos += 1 + n;
    return true;
}

size_t wye_functions_pack(const struct wye_functions *f, uint8_t *out)
{
    size_t n = 1;
    out[0] = f->flags;
    n += put(out + n, f->drive_vendor, sizeof(f->drive_vendor));
    n += put(out + n, f->drive_product, sizeof(f->drive_product));
    return n;
}

bool wye_functions_unpack(const uint8_t *p, size_t len, struct wye_functions *f)
{
    size_t pos = 1;
    if (len < 1) {
        return false;
    }
    memset(f, 0, sizeof(*f));
    f->flags = p[0];
    return get(p, len, &pos, f->drive_vendor, sizeof(f->drive_vendor)) &&
           get(p, len, &pos, f->drive_product, sizeof(f->drive_product));
}
