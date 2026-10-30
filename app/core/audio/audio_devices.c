/* Which sound devices are the bridge's, and which are this computer's own. */
#include "audio_internal.h"

#include "util/util.h"

#include <ctype.h>
#include <string.h>

/* Case-insensitive, with '_' matching ' ' (PulseAudio ids write "Wye_Bridge"). */
static bool loose_contains(const char *hay, const char *needle)
{
    size_t n = strlen(needle);
    for (; n && *hay; ++hay) {
        size_t i = 0;
        while (i < n && hay[i]) {
            char a = (char)tolower((unsigned char)hay[i]), b = (char)tolower((unsigned char)needle[i]);
            if (a != b && !((a == '_' || a == ' ') && (b == '_' || b == ' '))) {
                break;
            }
            ++i;
        }
        if (i == n) {
            return true;
        }
    }
    return false;
}

static bool is_bridge(ma_context *ctx, const ma_device_info *d, const char *bridge)
{
    const char *id = "";
    if (ctx->backend == ma_backend_pulseaudio) {
        id = d->id.pulse; /* e.g. alsa_output.usb-Wye_Wye_Bridge_WYE-BRIDGE-02.mono-fallback */
    } else if (ctx->backend == ma_backend_alsa) {
        id = d->id.alsa;
    }
    return bridge[0] && (loose_contains(d->name, bridge) || loose_contains(id, bridge));
}

static bool is_monitor(ma_context *ctx, const ma_device_info *d)
{
    return ctx->backend == ma_backend_pulseaudio && strstr(d->id.pulse, ".monitor") != NULL;
}

static void take(struct wye_audio_pick *p, const ma_device_info *d)
{
    p->found = true;
    p->id = d->id;
    wye_strcpy(p->name, sizeof(p->name), d->name);
}

/* want_pico: the first of the bridge's devices; else this computer's default (never a Pico's). */
static void find(ma_context *ctx, const ma_device_info *list, ma_uint32 n, const char *bridge, bool want_pico,
                 struct wye_audio_pick *out)
{
    memset(out, 0, sizeof(*out));
    for (ma_uint32 i = 0; i < n; ++i) {
        const ma_device_info *d = &list[i];
        bool pico = is_bridge(ctx, d, bridge) || is_bridge(ctx, d, "Wye ");
        if (is_monitor(ctx, d) || pico != want_pico || (out->found && (want_pico || !d->isDefault))) {
            continue;
        }
        take(out, d); /* for this computer: the system default, or else the first real device */
    }
}

void audio_pick(ma_context *ctx, const char *bridge, bool capture, struct wye_audio_pick *pico,
                struct wye_audio_pick *local)
{
    ma_device_info *play, *rec;
    ma_uint32 nplay, nrec;
    memset(pico, 0, sizeof(*pico));
    memset(local, 0, sizeof(*local));
    if (ma_context_get_devices(ctx, &play, &nplay, &rec, &nrec) != MA_SUCCESS) {
        return;
    }
    find(ctx, capture ? rec : play, capture ? nrec : nplay, bridge, true, pico);
    find(ctx, capture ? play : rec, capture ? nplay : nrec, bridge, false, local); /* other direction, here */
}
