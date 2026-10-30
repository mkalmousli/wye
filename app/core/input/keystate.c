#include "keystate.h"

#include "hid_usage.h"

#include <string.h>

void wye_keystate_clear(struct wye_keystate *k)
{
    memset(k, 0, sizeof(*k));
}

bool wye_keystate_has(const struct wye_keystate *k, uint8_t usage)
{
    if (HID_IS_MODIFIER(usage)) {
        return (k->modifiers & HID_MODIFIER_BIT(usage)) != 0;
    }
    return memchr(k->keys, usage, k->count) != NULL;
}

bool wye_keystate_press(struct wye_keystate *k, uint8_t usage)
{
    if (usage == 0 || wye_keystate_has(k, usage)) {
        return false;
    }
    if (HID_IS_MODIFIER(usage)) {
        k->modifiers |= (uint8_t)HID_MODIFIER_BIT(usage);
        return true;
    }
    if (k->count == WYE_KEYSTATE_MAX) {
        return false;
    }
    k->keys[k->count++] = usage;
    return true;
}

bool wye_keystate_release(struct wye_keystate *k, uint8_t usage)
{
    if (!wye_keystate_has(k, usage)) {
        return false;
    }
    if (HID_IS_MODIFIER(usage)) {
        k->modifiers &= (uint8_t)~HID_MODIFIER_BIT(usage);
        return true;
    }
    uint8_t *p = memchr(k->keys, usage, k->count);
    size_t idx = (size_t)(p - k->keys);
    memmove(p, p + 1, k->count - idx - 1u);
    k->count--;
    return true;
}

bool wye_keystate_empty(const struct wye_keystate *k)
{
    return k->modifiers == 0 && k->count == 0;
}

void wye_keystate_report(const struct wye_keystate *k, struct wye_keyboard *out)
{
    memset(out, 0, sizeof(*out));
    out->modifiers = k->modifiers;
    uint8_t first = k->count > 6 ? (uint8_t)(k->count - 6) : 0;
    for (uint8_t i = first; i < k->count; ++i) {
        out->keys[i - first] = k->keys[i];
    }
}
