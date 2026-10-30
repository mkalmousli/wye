/*
 * Devices > Sound: the two Picos are a sound card on each computer, and this
 * app connects the bridge's to this computer's own speakers and microphone.
 * Two lanes show each direction live, with its switch.
 */
#include "devices.h"
#include "ui_icons.h"
#include "ui_theme.h"

static const char *state(const struct wye_route_status *r, bool on)
{
    return !on ? "Off" : r->running ? "Live" : r->problem[0] ? r->problem : "Starting...";
}

static const char *or(const char *name, const char *fallback)
{
    return name[0] ? name : fallback;
}

void devices_audio(struct wye_ui *ui, Rectangle c)
{
    const struct wye_audio_status *au = &ui->st.audio;
    const struct wye_route_status *spk = &au->route[WYE_ROUTE_SPEAKER], *mic = &au->route[WYE_ROUTE_MIC];
    bool supported = (ui->st.link.target.hello.caps & WYE_CAP_AUDIO) &&
                     (ui->st.link.bridge.hello.caps & WYE_CAP_AUDIO);
    ui_text_bold(ui, "Sound between the computers", c.x, c.y, 22, UI_TEXT);
    ui_text(ui, supported ? "Wye connects everything on this computer - just pick the Pico on the other one."
                          : "Connect both Picos (firmware 3.3 or newer) to share sound.",
            c.x, c.y + 34, UI_FONT_SMALL, supported ? UI_MUTED : UI_WARN);
    struct devices_lane lanes[2] = {
        {"SPEAKER", state(spk, ui->cfg.use_speaker), UI_ICON_SPEAKER, "Other computer", "plays to the Pico",
         UI_ICON_SPEAKER, "This computer", or(spk->to, "your speakers"), UI_TARGET, UI_ACCENT2,
         ui->cfg.use_speaker, spk->running, spk->level},
        {"MICROPHONE", state(mic, ui->cfg.use_mic), UI_ICON_MIC, "This computer", or(mic->from, "your microphone"),
         UI_ICON_MIC, "Other computer", "records from the Pico", UI_ACCENT2, UI_TARGET2, ui->cfg.use_mic,
         mic->running, mic->level},
    };
    bool *field[2] = {&ui->cfg.use_speaker, &ui->cfg.use_mic};
    float lane_h = 118, y = c.y + 76;
    for (int i = 0; i < 2; ++i) {
        if (devices_audio_lane(ui, (Rectangle){c.x, y + (float)i * (lane_h + 14), c.width, lane_h}, &lanes[i])) {
            *field[i] = !*field[i];
            ui_save_config(ui); /* the sound worker follows; the target adds or drops its sound device */
        }
    }
    y += 2 * lane_h + 40;
    ui_text_bold(ui, "On the other computer", c.x, y, UI_FONT, UI_TEXT);
    ui_text(ui, "Choose the Pico as speaker and microphone in its sound settings. It carries the Pico's USB name",
            c.x, y + 28, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, "(see USB identity). Sound is 16 kHz mono - clear speech, low delay - and flows all the time.",
            c.x, y + 50, UI_FONT_SMALL, UI_MUTED);
}
