/* Pico names and colours, as lines of "<id in hex> <colour> <name>" in picos.ini. */
#include "pico_names.h"

#include "plat_sys.h"
#include "util/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAMES_FILE "picos.ini"
#define UID_HEX 16

static bool names_path(char *out, size_t len)
{
    char dir[PLAT_PATH_MAX];
    return plat_config_dir(dir, sizeof(dir)) && wye_path_join(out, len, dir, NAMES_FILE);
}

static bool has_id(const uint8_t uid[8])
{
    static const uint8_t NONE[8];
    return memcmp(uid, NONE, 8) != 0;
}

void wye_pico_name_clean(char *name)
{
    for (char *s = name; *s; ++s) {
        if ((unsigned char)*s < 0x20 || *s == 0x7f) {
            *s = ' ';
        }
    }
    char *start = wye_trim(name);
    memmove(name, start, strlen(start) + 1);
    size_t len = strlen(name);
    if (len > WYE_PICO_NAME_MAX) {
        len = WYE_PICO_NAME_MAX;
        while (len > 0 && ((unsigned char)name[len] & 0xc0) == 0x80) {
            len--; /* the middle of a character */
        }
        name[len] = '\0';
        start = wye_trim(name); /* the cut may leave a trailing space */
        memmove(name, start, strlen(start) + 1);
    }
}

const struct wye_pico_name *wye_pico_names_find(const struct wye_pico_names *n, const uint8_t uid[8])
{
    for (int i = 0; has_id(uid) && i < n->count; ++i) {
        if (memcmp(n->items[i].uid, uid, 8) == 0) {
            return &n->items[i];
        }
    }
    return NULL;
}

static void drop(struct wye_pico_names *n, int i)
{
    memmove(&n->items[i], &n->items[i + 1], (size_t)(n->count - i - 1) * sizeof(n->items[0]));
    n->count--;
}

bool wye_pico_names_set(struct wye_pico_names *n, const uint8_t uid[8], const char *name, int color)
{
    if (!has_id(uid)) {
        return false;
    }
    const struct wye_pico_name *old = wye_pico_names_find(n, uid);
    if (old) {
        drop(n, (int)(old - n->items));
    }
    struct wye_pico_name e;
    memset(&e, 0, sizeof(e));
    memcpy(e.uid, uid, 8);
    e.color = (uint8_t)(color > 0 && color < WYE_PICO_COLORS ? color : 0);
    char clean[WYE_PICO_NAME_MAX * 4 + 1];
    wye_strcpy(clean, sizeof(clean), name ? name : "");
    wye_pico_name_clean(clean);
    wye_strcpy(e.name, sizeof(e.name), clean);
    if (!e.name[0] && !e.color) {
        return true;
    }
    if (n->count == WYE_PICO_NAMES) {
        drop(n, 0);
    }
    n->items[n->count++] = e;
    return true;
}

void wye_pico_names_load(struct wye_pico_names *n)
{
    char path[PLAT_PATH_MAX];
    memset(n, 0, sizeof(*n));
    char *data = names_path(path, sizeof(path)) ? (char *)wye_read_file(path, NULL, 16 * 1024) : NULL;
    if (!data) {
        return;
    }
    for (char *line = strtok(data, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        uint8_t uid[8];
        char *color = strchr(line, ' ');
        char *name = color ? strchr(color + 1, ' ') : NULL;
        if (!name || color - line != UID_HEX) {
            continue;
        }
        *color++ = '\0';
        *name++ = '\0';
        if (wye_hex_decode(line, uid, sizeof(uid)) == sizeof(uid)) {
            wye_pico_names_set(n, uid, name, atoi(color));
        }
    }
    free(data);
}

bool wye_pico_names_save(const struct wye_pico_names *n)
{
    char path[PLAT_PATH_MAX], buf[WYE_PICO_NAMES * (UID_HEX + WYE_PICO_NAME_MAX + 8)];
    size_t used = 0;
    for (int i = 0; i < n->count; ++i) {
        char hex[UID_HEX + 1];
        wye_hex_encode(n->items[i].uid, 8, hex, sizeof(hex));
        used += (size_t)snprintf(buf + used, sizeof(buf) - used, "%s %d %s\n", hex, n->items[i].color, n->items[i].name);
    }
    return names_path(path, sizeof(path)) && wye_write_file_atomic(path, buf, used);
}
