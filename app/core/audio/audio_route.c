/*
 * One route: whatever one device records is played straight out of another.
 * The two devices run on different clocks (a USB Pico and a sound card), so
 * they are joined by a ring buffer kept near ROUTE_FILL frames: it refills after an
 * underrun and skips ahead when it grows past ROUTE_MAX, which hides clock drift.
 */
#include "audio_internal.h"

#include <math.h>
#include <string.h>

void audio_route_recorded(ma_device *dev, void *out, const void *in, ma_uint32 frames)
{
    (void)out;
    struct audio_route *r = dev->pUserData;
    const float *src = in;
    float peak = 0;
    for (ma_uint32 i = 0; i < frames; ++i) {
        peak = fmaxf(peak, fabsf(src[i]));
    }
    r->peak = fmaxf(peak, r->peak * 0.92f); /* the UI reads this as a level meter */
    r->recorded += frames;
    while (frames) {
        ma_uint32 n = frames;
        void *dst;
        if (ma_pcm_rb_acquire_write(&r->rb, &n, &dst) != MA_SUCCESS || n == 0) {
            return; /* full: the player skips ahead */
        }
        memcpy(dst, src, n * sizeof(float));
        ma_pcm_rb_commit_write(&r->rb, n);
        src += n;
        frames -= n;
    }
}

void audio_route_play(ma_device *dev, void *out, const void *in, ma_uint32 frames)
{
    (void)in;
    struct audio_route *r = dev->pUserData;
    float *dst = out;
    ma_uint32 have = ma_pcm_rb_available_read(&r->rb);
    if (have > ROUTE_MAX) {
        ma_pcm_rb_seek_read(&r->rb, have - ROUTE_FILL);
        r->skipped += have - ROUTE_FILL;
        have = ROUTE_FILL;
    }
    r->primed = r->primed ? have > 0 : have >= ROUTE_FILL;
    while (r->primed && frames) {
        ma_uint32 n = frames;
        void *src;
        if (ma_pcm_rb_acquire_read(&r->rb, &n, &src) != MA_SUCCESS || n == 0) {
            r->primed = false; /* ran dry: wait for ROUTE_FILL again */
            break;
        }
        memcpy(dst, src, n * sizeof(float));
        ma_pcm_rb_commit_read(&r->rb, n);
        r->played += n;
        dst += n;
        frames -= n;
    }
    memset(dst, 0, frames * sizeof(float));
}
