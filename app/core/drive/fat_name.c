/* FAT directory entries: long names (UCS-2) and generated 8.3 aliases. */
#include "fat.h"
#include "wye_msg.h"
#include <stdio.h>
#include <string.h>

static const uint8_t LFN_POS[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};

/* UTF-8 -> UCS-2; characters FAT cannot hold become '_'. */
static size_t to_ucs2(const char *s, uint16_t *out, size_t cap)
{
    size_t n = 0;
    for (const unsigned char *p = (const unsigned char *)s; *p && n < cap;) {
        uint32_t c = *p++;
        int extra = c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : c >= 0xc0 ? 1 : 0;
        c &= extra == 3 ? 0x07 : extra == 2 ? 0x0f : extra == 1 ? 0x1f : 0x7f;
        for (; extra > 0 && (*p & 0xc0) == 0x80; --extra) {
            c = (c << 6) | (*p++ & 0x3f);
        }
        bool bad = c < 0x20 || c > 0xffff || strchr("\\/:*?\"<>|", (int)(c < 128 ? c : 'a'));
        out[n++] = bad ? '_' : (uint16_t)c;
    }
    return n;
}

int fat_lfn_slots(const char *utf8)
{
    uint16_t buf[255];
    return (int)((to_ucs2(utf8, buf, 255) + 12) / 13);
}

static uint8_t checksum(const uint8_t *name)
{
    uint8_t sum = 0;
    for (int i = 0; i < 11; ++i) {
        sum = (uint8_t)(((sum & 1) << 7) + (sum >> 1) + name[i]);
    }
    return sum;
}

/* "H000123 TXT": unique per directory through the index; the long name is what users see. */
static void short_name(uint8_t *out, const char *utf8, unsigned index)
{
    char base[9];
    snprintf(base, sizeof(base), "H%06X", index & 0xffffffu);
    memset(out, ' ', 11);
    memcpy(out, base, 7);
    const char *dot = strrchr(utf8, '.');
    for (int i = 0, o = 8; dot && dot[1 + i] && o < 11; ++i) {
        char c = dot[1 + i];
        c = (char)(c >= 'a' && c <= 'z' ? c - 32 : c);
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            out[o++] = (uint8_t)c;
        }
    }
}

static void stamp(uint8_t *e, time_t t)
{
    struct tm tm_v = *localtime(&t);
    uint16_t date = (uint16_t)(((tm_v.tm_year > 80 ? tm_v.tm_year - 80 : 0) << 9) | ((tm_v.tm_mon + 1) << 5) | tm_v.tm_mday);
    uint16_t tod = (uint16_t)((tm_v.tm_hour << 11) | (tm_v.tm_min << 5) | (tm_v.tm_sec / 2));
    wye_put_u16(e + 14, tod);
    wye_put_u16(e + 16, date);
    wye_put_u16(e + 18, date);
    wye_put_u16(e + 22, tod);
    wye_put_u16(e + 24, date);
}

void fat_make_entries(uint8_t *out, const char *utf8, unsigned index, uint8_t attr, uint32_t cluster,
                      uint32_t size, time_t mtime)
{
    uint16_t name[255];
    size_t n = to_ucs2(utf8, name, 255);
    int slots = (int)((n + 12) / 13);
    uint8_t *e = out + (size_t)slots * FAT_DIRENT;
    memset(out, 0, (size_t)(slots + 1) * FAT_DIRENT);
    short_name(e, utf8, index);
    for (int k = slots; k >= 1; --k) {
        uint8_t *l = out + (size_t)(slots - k) * FAT_DIRENT;
        l[0] = (uint8_t)(k | (k == slots ? 0x40 : 0));
        l[11] = FAT_ATTR_LFN;
        l[13] = checksum(e);
        for (int i = 0; i < 13; ++i) {
            size_t at = (size_t)(k - 1) * 13 + (size_t)i;
            wye_put_u16(l + LFN_POS[i], at < n ? name[at] : at == n ? 0x0000 : 0xffff);
        }
    }
    e[11] = attr;
    stamp(e, mtime);
    wye_put_u16(e + 20, (uint16_t)(cluster >> 16)); /* FAT32 high half */
    wye_put_u16(e + 26, (uint16_t)cluster);
    wye_put_u32(e + 28, size);
}
