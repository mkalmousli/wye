/* Bounded PCM jitter buffer. All calls run on the USB/main task, never an ISR. */
#ifndef WYE_AUDIO_QUEUE_H
#define WYE_AUDIO_QUEUE_H

#include "wye_audio.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WYE_AUDIO_QUEUE_BYTES 512u
#define WYE_AUDIO_PREFILL_BYTES (6u * WYE_AUDIO_FRAME_BYTES) /* 6 ms: UART packets carry 2 ms */
#define WYE_AUDIO_SLACK_BYTES (2u * WYE_AUDIO_FRAME_BYTES)   /* no rate change within +-2 ms */
#define WYE_AUDIO_STALE_MS 20u

struct wye_audio_queue {
    uint8_t data[WYE_AUDIO_QUEUE_BYTES];
    uint16_t head, count;
    uint32_t received_at;
    bool playing;
};

void wye_audio_queue_reset(struct wye_audio_queue *q);
bool wye_audio_queue_push(struct wye_audio_queue *q, const uint8_t *data, size_t len, uint32_t now);
/* One USB frame, silence on starvation. Capacity must be WYE_AUDIO_USB_MAX_BYTES.
   Vary the frame by one sample to track the other computer's USB clock. */
size_t wye_audio_queue_frame(struct wye_audio_queue *q, uint8_t *out, uint32_t now);

#endif
