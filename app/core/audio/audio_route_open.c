/* A route's life: opening its two devices and ring buffer, closing them, noticing a device went away. */
#include "audio_internal.h"

#include "util/util.h"

static ma_result open_one(struct audio_route *r, ma_context *ctx, ma_device *dev, bool capture,
                          const ma_device_id *id)
{
    ma_device_config c = ma_device_config_init(capture ? ma_device_type_capture : ma_device_type_playback);
    c.capture.pDeviceID = c.playback.pDeviceID = id;
    c.capture.format = c.playback.format = ma_format_f32;
    c.capture.channels = c.playback.channels = 1;
    c.sampleRate = ROUTE_RATE;
    c.periodSizeInMilliseconds = 10;
    c.dataCallback = capture ? audio_route_recorded : audio_route_play;
    c.pUserData = r;
    return ma_device_init(ctx, &c, dev);
}

bool audio_route_open(struct audio_route *r, ma_context *ctx, const ma_device_id *from, const ma_device_id *to,
                      char *err, size_t errlen)
{
    r->peak = 0;
    r->primed = false;
    ma_result res = ma_pcm_rb_init(ma_format_f32, 1, ROUTE_MAX * 2, NULL, NULL, &r->rb);
    if (res != MA_SUCCESS) {
        wye_strcpy(err, errlen, ma_result_description(res));
        return false;
    }
    if ((res = open_one(r, ctx, &r->in, true, from)) == MA_SUCCESS) {
        if ((res = open_one(r, ctx, &r->out, false, to)) == MA_SUCCESS) {
            if ((res = ma_device_start(&r->out)) == MA_SUCCESS && (res = ma_device_start(&r->in)) == MA_SUCCESS) {
                r->open = true;
                return true;
            }
            ma_device_uninit(&r->out);
        }
        ma_device_uninit(&r->in);
    }
    ma_pcm_rb_uninit(&r->rb);
    wye_strcpy(err, errlen, ma_result_description(res));
    return false;
}


void audio_route_close(struct audio_route *r)
{
    if (r->open) {
        ma_device_uninit(&r->in);
        ma_device_uninit(&r->out);
        ma_pcm_rb_uninit(&r->rb);
        r->open = false;
    }
    r->peak = 0;
}

/* False once either device stopped (unplugged, sound server restarted). */
bool audio_route_alive(struct audio_route *r)
{
    return r->open && ma_device_get_state(&r->in) == ma_device_state_started &&
           ma_device_get_state(&r->out) == ma_device_state_started;
}
