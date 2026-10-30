/* Key names for the `key` command. */
#include "api.h"

#include "features/typing.h"
#include "hid_usage.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct {
    const char *name;
    uint8_t key;
} NAMES[] = {
    {"enter", HID_KEY_ENTER}, {"return", HID_KEY_ENTER}, {"esc", HID_KEY_ESCAPE}, {"escape", HID_KEY_ESCAPE},
    {"backspace", HID_KEY_BACKSPACE}, {"tab", HID_KEY_TAB}, {"space", HID_KEY_SPACE}, {"capslock", HID_KEY_CAPS_LOCK},
    {"printscreen", 0x46}, {"scrolllock", HID_KEY_SCROLL_LOCK}, {"pause", HID_KEY_PAUSE}, {"insert", HID_KEY_INSERT},
    {"home", 0x4a}, {"pageup", 0x4b}, {"delete", HID_KEY_DELETE}, {"del", HID_KEY_DELETE}, {"end", 0x4d},
    {"pagedown", 0x4e}, {"right", 0x4f}, {"left", 0x50}, {"down", 0x51}, {"up", 0x52}, {"numlock", HID_KEY_NUM_LOCK},
    {"menu", 0x65},
};

static const struct {
    const char *name;
    uint8_t usage;
} MODIFIERS[] = {
    {"ctrl", HID_KEY_LEFT_CTRL}, {"control", HID_KEY_LEFT_CTRL}, {"shift", HID_KEY_LEFT_SHIFT}, {"alt", HID_KEY_LEFT_ALT},
    {"super", HID_KEY_LEFT_GUI}, {"win", HID_KEY_LEFT_GUI}, {"cmd", HID_KEY_LEFT_GUI}, {"meta", HID_KEY_LEFT_GUI},
    {"rctrl", HID_KEY_RIGHT_CTRL}, {"rshift", HID_KEY_RIGHT_SHIFT}, {"ralt", HID_KEY_RIGHT_ALT}, {"altgr", HID_KEY_RIGHT_ALT},
    {"rsuper", HID_KEY_RIGHT_GUI},
};

static bool same(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) {
            return false;
        }
    }
    return b[n] == '\0';
}

static bool function_key(const char *s, size_t n, uint8_t *key)
{
    if (n < 2 || n > 3 || (s[0] != 'f' && s[0] != 'F') || !isdigit((unsigned char)s[1]) ||
        (n == 3 && !isdigit((unsigned char)s[2]))) {
        return false;
    }
    int f = n == 2 ? s[1] - '0' : (s[1] - '0') * 10 + (s[2] - '0');
    if (f < 1 || f > 24) {
        return false;
    }
    *key = (uint8_t)(f <= 12 ? HID_KEY_F1 + f - 1 : HID_KEY_F13 + f - 13);
    return true;
}

static bool one_part(const char *s, size_t n, uint8_t *modifiers, uint8_t *key)
{
    for (size_t i = 0; i < sizeof(MODIFIERS) / sizeof(MODIFIERS[0]); ++i) {
        if (same(s, MODIFIERS[i].name, n)) {
            *modifiers |= (uint8_t)HID_MODIFIER_BIT(MODIFIERS[i].usage);
            return true;
        }
    }
    if (*key) {
        return false; /* a chord has one key */
    }
    for (size_t i = 0; i < sizeof(NAMES) / sizeof(NAMES[0]); ++i) {
        if (same(s, NAMES[i].name, n)) {
            *key = NAMES[i].key;
            return true;
        }
    }
    if (function_key(s, n, key)) {
        return true;
    }
    uint8_t shift = 0;
    if (n == 1 && wye_ascii_to_hid(s[0], &shift, key) && s[0] != ' ' && s[0] != '\t' && s[0] != '\n') {
        *modifiers |= shift;
        return true;
    }
    *key = 0;
    return false;
}

bool wye_api_parse_chord(const char *text, uint8_t *modifiers, uint8_t *key)
{
    *modifiers = 0;
    *key = 0;
    if (!*text) {
        return false;
    }
    for (const char *s = text;;) {
        const char *plus = strchr(s + (*s == '+' ? 1 : 0), '+'); /* a leading '+' is the key itself */
        size_t n = plus ? (size_t)(plus - s) : strlen(s);
        if (n == 0 || !one_part(s, n, modifiers, key)) {
            return false;
        }
        if (!plus) {
            return true;
        }
        s = plus + 1;
    }
}
