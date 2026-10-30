/* The PCM jitter buffer that feeds each Pico's USB microphone. */
#include "audio_queue.h"
#include "test.h"

#include <string.h>

TEST(audio_queue_prefills_and_silences_on_cable_loss)
{
    struct wye_audio_queue q = {0};
    uint8_t pcm[WYE_AUDIO_CHUNK], out[WYE_AUDIO_USB_MAX_BYTES], silence[WYE_AUDIO_FRAME_BYTES] = {0};
    memset(pcm, 0x3f, sizeof(pcm));
    CHECK(wye_audio_queue_push(&q, pcm, 32, 0));
    CHECK_EQ(wye_audio_queue_frame(&q, out, 0), sizeof(silence));
    CHECK_MEM(out, silence, sizeof(silence));
    CHECK_EQ(q.count, 32); /* retain startup data until prefilled */
    CHECK(wye_audio_queue_push(&q, pcm, 96, 1));
    CHECK(wye_audio_queue_push(&q, pcm, WYE_AUDIO_PREFILL_BYTES - 128, 1));
    CHECK_EQ(wye_audio_queue_frame(&q, out, 1), 32);
    CHECK_MEM(out, pcm, 32);
    CHECK_EQ(wye_audio_queue_frame(&q, out, WYE_AUDIO_STALE_MS + 2), sizeof(silence));
    CHECK_MEM(out, silence, sizeof(silence));
    CHECK_EQ(q.count, 0);
    CHECK(!q.playing);
    CHECK(!wye_audio_queue_push(&q, pcm, 3, 30));
    CHECK(!wye_audio_queue_push(&q, pcm, WYE_AUDIO_CHUNK + 2, 30));
}

TEST(audio_queue_bounds_latency_and_preserves_sample_alignment)
{
    struct wye_audio_queue q = {0};
    uint8_t pcm[WYE_AUDIO_CHUNK], out[WYE_AUDIO_USB_MAX_BYTES];
    for (unsigned i = 0; i < 5; ++i) {
        memset(pcm, (int)i + 1, sizeof(pcm));
        CHECK(wye_audio_queue_push(&q, pcm, sizeof(pcm), i));
    }
    CHECK_EQ(q.count, WYE_AUDIO_QUEUE_BYTES);
    CHECK_EQ(wye_audio_queue_frame(&q, out, 5), WYE_AUDIO_USB_MAX_BYTES);
    CHECK_EQ(out[0], 2); /* oldest chunk was dropped */
    CHECK_EQ(out[1], 2);
    for (unsigned i = 0; i < 30; ++i) {
        size_t n = wye_audio_queue_frame(&q, out, 5);
        CHECK_EQ(n % WYE_AUDIO_SAMPLE_BYTES, 0);
        CHECK(n <= sizeof(out));
    }
    CHECK(!q.playing);
    CHECK_EQ(q.count, 0);
}

TEST(audio_queue_tracks_different_usb_clocks)
{
    uint8_t pcm[WYE_AUDIO_CHUNK], out[WYE_AUDIO_USB_MAX_BYTES];
    memset(pcm, 0x7e, sizeof(pcm));
    for (int drift = -1; drift <= 1; drift += 2) {
        struct wye_audio_queue q = {0};
        CHECK(wye_audio_queue_push(&q, pcm, WYE_AUDIO_PREFILL_BYTES / 2, 0));
        CHECK(wye_audio_queue_push(&q, pcm, WYE_AUDIO_PREFILL_BYTES / 2, 0));
        for (unsigned ms = 1; ms < 3000; ++ms) { /* the peer sends 2 ms per packet */
            size_t received = ms % 10 == 0 ? (size_t)(64 + drift * 2) : 64;
            if (ms % 2 == 0) CHECK(wye_audio_queue_push(&q, pcm, received, ms));
            size_t sent = wye_audio_queue_frame(&q, out, ms);
            CHECK(sent == 30 || sent == 32 || sent == 34);
            for (size_t i = 0; i < sent; ++i) CHECK_EQ(out[i], 0x7e);
            CHECK(q.count < WYE_AUDIO_PREFILL_BYTES + 4 * WYE_AUDIO_FRAME_BYTES);
            CHECK(q.playing);
        }
    }
}
