/*
 * Live activity for the dashboard: the keys recently sent to the target,
 * pointer motion since the UI last looked, and a count of every message
 * sent (input, jiggler, typing) so links can visibly carry traffic.
 */
#ifndef WYE_ACTIVITY_H
#define WYE_ACTIVITY_H

#include <stdbool.h>
#include <stdint.h>

#define WYE_ACTIVITY_KEYS 10

struct wye_activity {
    uint16_t keys[WYE_ACTIVITY_KEYS];  /* HID usages, oldest first */
    uint64_t key_at[WYE_ACTIVITY_KEYS];
    int key_count;
    int32_t dx, dy, wheel;             /* forwarded since the last take */
    uint8_t buttons;                   /* held on the target */
    uint32_t messages;                 /* total sent to the target */
};

void wye_activity_key(struct wye_activity *a, uint16_t usage, uint64_t now);
/* Copies the activity out and restarts the motion accumulators. */
void wye_activity_take(struct wye_activity *a, struct wye_activity *out);

#endif
