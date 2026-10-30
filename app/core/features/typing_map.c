/* ASCII to HID usage for a US keyboard layout. */
#include "typing.h"

#include "hid_usage.h"

#include <string.h>

#define SHIFT 0x02u

/* Punctuation: unshifted and shifted characters per usage. */
static const struct {
    char plain, shifted;
    uint8_t key;
} PUNCT[] = {
    {'-', '_', 0x2d}, {'=', '+', 0x2e}, {'[', '{', 0x2f}, {']', '}', 0x30},
    {'\\', '|', 0x31}, {';', ':', 0x33}, {'\'', '"', 0x34}, {'`', '~', 0x35},
    {',', '<', 0x36}, {'.', '>', 0x37}, {'/', '?', 0x38},
};

static const char SHIFTED_DIGITS[] = ")!@#$%^&*(";

bool wye_ascii_to_hid(char c, uint8_t *modifiers, uint8_t *key)
{
    *modifiers = 0;
    if (c >= 'a' && c <= 'z') {
        *key = (uint8_t)(HID_KEY_A + (c - 'a'));
        return true;
    }
    if (c >= 'A' && c <= 'Z') {
        *modifiers = SHIFT;
        *key = (uint8_t)(HID_KEY_A + (c - 'A'));
        return true;
    }
    if (c >= '0' && c <= '9') {
        *key = c == '0' ? HID_KEY_0 : (uint8_t)(HID_KEY_1 + (c - '1'));
        return true;
    }
    const char *sd = c ? strchr(SHIFTED_DIGITS, c) : NULL;
    if (sd) {
        int digit = (int)(sd - SHIFTED_DIGITS);
        *modifiers = SHIFT;
        *key = digit == 0 ? HID_KEY_0 : (uint8_t)(HID_KEY_1 + digit - 1);
        return true;
    }
    switch (c) {
    case ' ': *key = HID_KEY_SPACE; return true;
    case '\n': case '\r': *key = HID_KEY_ENTER; return true;
    case '\t': *key = HID_KEY_TAB; return true;
    default: break;
    }
    for (size_t i = 0; i < sizeof(PUNCT) / sizeof(PUNCT[0]); ++i) {
        if (c == PUNCT[i].plain || c == PUNCT[i].shifted) {
            *modifiers = c == PUNCT[i].shifted ? SHIFT : 0;
            *key = PUNCT[i].key;
            return true;
        }
    }
    return false;
}
