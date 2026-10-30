/* The sound worker's insides: device picking, routes, shared state (audio*.c only: pulls in miniaudio). */
#ifndef WYE_AUDIO_INTERNAL_H
#define WYE_AUDIO_INTERNAL_H

#include "audio.h"

#include "miniaudio.h"

struct wye_audio_pick {
    bool found;
    ma_device_id id;
    char name[64];
};
/* The bridge's capture (capture) or playback device, and this computer's default device for
   the other direction - what a route connects it to. */
void audio_pick(ma_context *ctx, const char *bridge, bool capture, struct wye_audio_pick *pico,
                struct wye_audio_pick *local);

/* A route joins two clocks through a ring buffer kept near FILL frames; past MAX it skips ahead. */
#define ROUTE_RATE 48000
#define ROUTE_FILL (ROUTE_RATE * 30 / 1000)  /* 30 ms buffered */
#define ROUTE_MAX (ROUTE_RATE * 90 / 1000)   /* beyond this the newest audio wins */

struct audio_route {
    ma_device in, out;        /* recorded on one, played on the other (audio_route.c) */
    ma_pcm_rb rb;
    bool open;
    volatile bool primed;     /* playing; false while (re)filling */
    volatile float peak;
    volatile unsigned long recorded, played, skipped;  /* frames, for diagnosis */
};
bool audio_route_open(struct audio_route *r, ma_context *ctx, const ma_device_id *from, const ma_device_id *to,
                      char *err, size_t errlen);
void audio_route_close(struct audio_route *r);
/* The two device callbacks (audio_route.c): recording fills the ring, playing drains it. */
void audio_route_recorded(ma_device *dev, void *out, const void *in, ma_uint32 frames);
void audio_route_play(ma_device *dev, void *out, const void *in, ma_uint32 frames);
bool audio_route_alive(struct audio_route *r);

#include "plat_thread.h"
/* State shared by the worker (audio.c) and the API (audio_api.c); lock guards bridge/want/st. */
struct audio_shared {
    plat_mutex *lock;
    plat_thread *thread;
    plat_event *wake;
    volatile bool stop;
    char bridge[64];
    bool want[WYE_ROUTE_COUNT];
    struct wye_audio_status st;
    struct audio_route route[WYE_ROUTE_COUNT];
    ma_device_id from[WYE_ROUTE_COUNT], to[WYE_ROUTE_COUNT];
};
extern struct audio_shared g_audio;
void audio_worker(void *arg);

#endif
