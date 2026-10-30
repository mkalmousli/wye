/*
 * Types text on the target as paced key presses (clipboard paste, macros).
 * Uses the US layout; characters without a mapping are skipped.
 */
#ifndef WYE_TYPING_H
#define WYE_TYPING_H

#include "sender.h"

#define WYE_TYPING_QUEUE 16384
#define WYE_TYPING_STEP_MS 6

struct wye_typing_step {
    uint8_t modifiers;
    uint8_t key;
};

struct wye_typing {
    struct wye_typing_step steps[WYE_TYPING_QUEUE];
    size_t head, count;
    uint64_t next_at;
};

/* Queues each character as press + release; returns characters queued. */
size_t wye_typing_text(struct wye_typing *t, const char *text);
/* Queues one chord (e.g. Ctrl+Alt+Del) as press + release. */
bool wye_typing_chord(struct wye_typing *t, uint8_t modifiers, uint8_t key);
void wye_typing_tick(struct wye_typing *t, uint64_t now, const struct wye_sender *out);
void wye_typing_cancel(struct wye_typing *t);
bool wye_typing_busy(const struct wye_typing *t);

/* US-layout lookup (typing_map.c). */
bool wye_ascii_to_hid(char c, uint8_t *modifiers, uint8_t *key);

#endif
