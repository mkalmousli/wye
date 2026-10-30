/* Set of pressed keys, convertible to a 6-key boot keyboard report. */
#ifndef WYE_KEYSTATE_H
#define WYE_KEYSTATE_H

#include "wye_msg.h"

#include <stdbool.h>
#include <stdint.h>

#define WYE_KEYSTATE_MAX 32

struct wye_keystate {
    uint8_t modifiers;
    uint8_t keys[WYE_KEYSTATE_MAX];   /* non-modifier usages in press order */
    uint8_t count;
};

void wye_keystate_clear(struct wye_keystate *k);
/* Returns true if the state changed. */
bool wye_keystate_press(struct wye_keystate *k, uint8_t usage);
bool wye_keystate_release(struct wye_keystate *k, uint8_t usage);
bool wye_keystate_has(const struct wye_keystate *k, uint8_t usage);
bool wye_keystate_empty(const struct wye_keystate *k);
/* With more than six keys the six most recent are reported. */
void wye_keystate_report(const struct wye_keystate *k, struct wye_keyboard *out);

/* 256-bit usage set (keys passed through to the local OS). */
struct wye_usage_set {
    uint32_t bits[8];
};

static inline void wye_usage_set(struct wye_usage_set *s, uint8_t u, bool on)
{
    if (on) {
        s->bits[u >> 5] |= 1u << (u & 31);
    } else {
        s->bits[u >> 5] &= ~(1u << (u & 31));
    }
}

static inline bool wye_usage_has(const struct wye_usage_set *s, uint8_t u)
{
    return (s->bits[u >> 5] >> (u & 31)) & 1u;
}

#endif
