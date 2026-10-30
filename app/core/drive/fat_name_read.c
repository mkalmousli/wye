/* FAT directory entries, reading side: "." / "..", long names, 8.3 names. */
#include "fat.h"
#include "wye_msg.h"
#include <string.h>

static const uint8_t LFN_POS[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};

void fat_dot_entries(uint8_t *out, uint32_t self, uint32_t parent)
{
    memset(out, 0, 2 * FAT_DIRENT);
    memcpy(out, ".          ", 11);
    memcpy(out + FAT_DIRENT, "..         ", 11);
    out[11] = out[FAT_DIRENT + 11] = FAT_ATTR_DIR;
    wye_put_u16(out + 20, (uint16_t)(self >> 16));
    wye_put_u16(out + 26, (uint16_t)self);
    wye_put_u16(out + FAT_DIRENT + 20, (uint16_t)(parent >> 16));
    wye_put_u16(out + FAT_DIRENT + 26, (uint16_t)parent); /* 0 = root, on FAT32 too */
}

void fat_lfn_add(struct fat_lfn *l, const uint8_t *e)
{
    uint8_t ord = e[0] & 0x1f;
    if (e[0] & 0x40) {
        memset(l->chars, 0, sizeof(l->chars));
        l->checksum = e[13];
        l->next = ord;
        l->valid = ord >= 1 && ord <= 20;
    }
    if (!l->valid || ord != l->next || e[13] != l->checksum) {
        l->valid = false;
        return;
    }
    for (int i = 0; i < 13; ++i) {
        l->chars[(ord - 1) * 13 + i] = wye_get_u16(e + LFN_POS[i]);
    }
    l->next = (uint8_t)(ord - 1); /* 0 once the whole name has arrived */
}

static size_t put_utf8(char *out, size_t o, size_t cap, uint16_t c)
{
    unsigned char bytes[3];
    size_t n = c < 0x80 ? 1 : c < 0x800 ? 2 : 3;
    bytes[0] = n == 1 ? (unsigned char)c : n == 2 ? (unsigned char)(0xc0 | (c >> 6)) : (unsigned char)(0xe0 | (c >> 12));
    bytes[1] = n == 2 ? (unsigned char)(0x80 | (c & 0x3f)) : (unsigned char)(0x80 | ((c >> 6) & 0x3f));
    bytes[2] = (unsigned char)(0x80 | (c & 0x3f));
    if (o + n >= cap) {
        return o;
    }
    memcpy(out + o, bytes, n);
    return o + n;
}

static uint8_t checksum(const uint8_t *name)
{
    uint8_t sum = 0;
    for (int i = 0; i < 11; ++i) {
        sum = (uint8_t)(((sum & 1) << 7) + (sum >> 1) + name[i]);
    }
    return sum;
}

void fat_entry_name(const uint8_t *e, const struct fat_lfn *l, char *out, size_t cap)
{
    size_t o = 0;
    if (l->valid && l->next == 0 && l->checksum == checksum(e)) {
        for (int i = 0; i < 260 && l->chars[i] && l->chars[i] != 0xffff; ++i) {
            o = put_utf8(out, o, cap, l->chars[i]);
        }
    } else {
        bool lower_base = e[12] & 0x08, lower_ext = e[12] & 0x10; /* Windows NT case flags */
        for (int i = 0; i < 11 && o + 2 < cap; ++i) {
            char c = (char)(i == 0 && e[0] == 0x05 ? 0xe5 : e[i]);
            if (i == 8 && e[8] != ' ') {
                out[o++] = '.';
            }
            if (c != ' ') {
                bool lower = i < 8 ? lower_base : lower_ext;
                out[o++] = (char)(lower && c >= 'A' && c <= 'Z' ? c + 32 : c);
            }
        }
    }
    out[o] = '\0';
}

time_t fat_entry_mtime(const uint8_t *e)
{
    uint16_t tod = wye_get_u16(e + 22), date = wye_get_u16(e + 24);
    struct tm t = {0};
    t.tm_year = 80 + (date >> 9);
    t.tm_mon = ((date >> 5) & 15) - 1;
    t.tm_mday = date & 31;
    t.tm_hour = tod >> 11;
    t.tm_min = (tod >> 5) & 63;
    t.tm_sec = (tod & 31) * 2;
    t.tm_isdst = -1;
    return date ? mktime(&t) : 0;
}
