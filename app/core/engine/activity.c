/* Live activity bookkeeping (engine lock held). */
#include "activity.h"
#include <string.h>

void wye_activity_key(struct wye_activity *a, uint16_t usage, uint64_t now)
{
    if (a->key_count == WYE_ACTIVITY_KEYS) {
        memmove(a->keys, a->keys + 1, sizeof(a->keys) - sizeof(a->keys[0]));
        memmove(a->key_at, a->key_at + 1, sizeof(a->key_at) - sizeof(a->key_at[0]));
        a->key_count--;
    }
    a->keys[a->key_count] = usage;
    a->key_at[a->key_count++] = now;
}

void wye_activity_take(struct wye_activity *a, struct wye_activity *out)
{
    *out = *a;
    a->dx = a->dy = a->wheel = 0;
}
