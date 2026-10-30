/*
 * Sound between the computers, this side. The bridge Pico is a sound card here;
 * Wye connects it to this computer's own speakers and microphone, so nobody
 * has to pick devices:
 *   speaker: bridge input ("Wye Microphone") -> this computer's speakers
 *   mic:     this computer's microphone -> bridge output ("Wye Speaker")
 * Runs on its own thread (opening devices can take a while); the engine only
 * says what it wants.
 */
#ifndef WYE_AUDIO_H
#define WYE_AUDIO_H

#include <stdbool.h>

enum wye_route { WYE_ROUTE_SPEAKER = 0, WYE_ROUTE_MIC, WYE_ROUTE_COUNT };

struct wye_route_status {
    bool wanted, running;
    float level;              /* 0..1, peak of the last few milliseconds */
    char from[64], to[64];    /* device names, as this computer calls them */
    char problem[96];         /* why it is not running, "" if it is */
};

struct wye_audio_status {
    struct wye_route_status route[WYE_ROUTE_COUNT];
};

void wye_audio_start(void);
void wye_audio_stop(void);
/* bridge: the bridge's USB product name ("" = not connected). */
void wye_audio_want(const char *bridge, bool speaker, bool mic);
void wye_audio_status(struct wye_audio_status *out);

#endif
