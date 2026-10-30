#include "audio_queue.h"

#include <string.h>

void wye_audio_queue_reset(struct wye_audio_queue *q)
{
    memset(q, 0, sizeof(*q));
}

bool wye_audio_queue_push(struct wye_audio_queue *q, const uint8_t *data, size_t len, uint32_t now)
{
    if (!len || len > WYE_AUDIO_CHUNK || len % WYE_AUDIO_SAMPLE_BYTES) {
        return false;
    }
    if (now - q->received_at > WYE_AUDIO_STALE_MS) {
        wye_audio_queue_reset(q);
    }
    /* Drop oldest samples on overflow: latency must stay bounded. */
    if (q->count + len > WYE_AUDIO_QUEUE_BYTES) {
        size_t drop = q->count + len - WYE_AUDIO_QUEUE_BYTES;
        q->head = (uint16_t)((q->head + drop) % WYE_AUDIO_QUEUE_BYTES);
        q->count -= (uint16_t)drop;
    }
    for (size_t i = 0; i < len; ++i) {
        q->data[(q->head + q->count + i) % WYE_AUDIO_QUEUE_BYTES] = data[i];
    }
    q->count += (uint16_t)len;
    q->received_at = now;
    return true;
}

size_t wye_audio_queue_frame(struct wye_audio_queue *q, uint8_t *out, uint32_t now)
{
    size_t len = WYE_AUDIO_FRAME_BYTES;
    if (now - q->received_at > WYE_AUDIO_STALE_MS) {
        wye_audio_queue_reset(q);
    }
    if (!q->playing && q->count >= WYE_AUDIO_PREFILL_BYTES) {
        q->playing = true;
    }
    if (q->playing) {
        if (q->count > WYE_AUDIO_PREFILL_BYTES + WYE_AUDIO_SLACK_BYTES) {
            len += WYE_AUDIO_SAMPLE_BYTES;
        } else if (q->count < WYE_AUDIO_PREFILL_BYTES - WYE_AUDIO_SLACK_BYTES) {
            len -= WYE_AUDIO_SAMPLE_BYTES;
        }
        if (q->count >= len) {
            for (size_t i = 0; i < len; ++i) {
                out[i] = q->data[(q->head + i) % WYE_AUDIO_QUEUE_BYTES];
            }
            q->head = (uint16_t)((q->head + len) % WYE_AUDIO_QUEUE_BYTES);
            q->count -= (uint16_t)len;
            return len;
        }
        wye_audio_queue_reset(q);
    }
    memset(out, 0, WYE_AUDIO_FRAME_BYTES);
    return WYE_AUDIO_FRAME_BYTES;
}
