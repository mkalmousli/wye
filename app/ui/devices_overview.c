/* Devices > Overview: switch each function on or off on its own - it takes effect at once. */
#include "devices.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include <math.h>

static const char *route_status(const struct wye_route_status *r, const char *on)
{
    return r->running ? on : r->problem[0] ? r->problem : "Starting...";
}

void devices_overview(struct wye_ui *ui, Rectangle a)
{
    const struct wye_audio_status *au = &ui->st.audio;
    bool sound = (ui->st.link.target.hello.caps & WYE_CAP_AUDIO) && (ui->st.link.bridge.hello.caps & WYE_CAP_AUDIO);
    bool drive = ui->st.link.target.hello.caps & WYE_CAP_DISK;
    bool fwd = ui->st.mode != WYE_FWD_OFF;
    struct devices_tile t[5] = {
        {UI_ICON_KEYBOARD, "Keyboard", "Your keys type on the other computer while you control it.",
         ui->cfg.use_keyboard, true, ui->cfg.use_keyboard ? (fwd ? "Typing there now" : "Ready") : "Keys stay here",
         -1, UI_ACCENT},
        {UI_ICON_JIGGLE, "Mouse", "Your mouse moves the other computer's pointer while you control it.",
         ui->cfg.use_mouse, true, ui->cfg.use_mouse ? (fwd ? "Pointing there now" : "Ready") : "Mouse stays here",
         -1, UI_ACCENT2},
        {UI_ICON_FOLDER, "Shared drive", "One folder both computers work in - a drive there, a folder here.",
         ui->cfg.drive_enabled, drive, ui->cfg.drive_enabled ? ui->st.drive.message : "Off", -1, UI_OK},
        {UI_ICON_SPEAKER, "Speaker", "Hear the other computer's sound on this computer's speakers.",
         ui->cfg.use_speaker, sound, route_status(&au->route[WYE_ROUTE_SPEAKER], "Playing here"),
         au->route[WYE_ROUTE_SPEAKER].level, UI_TARGET},
        {UI_ICON_MIC, "Microphone", "Your microphone becomes the other computer's microphone.",
         ui->cfg.use_mic, sound, route_status(&au->route[WYE_ROUTE_MIC], "Sending your voice"),
         au->route[WYE_ROUTE_MIC].level, UI_TARGET2},
    };
    if (!sound) {
        t[3].status = t[4].status = "Needs firmware 3.3 on both Picos";
    }
    /* input and the drive on top, the two directions of sound below, wider */
    float gap = 14, h = fminf(220, (a.height - gap - 30) / 2);
    float w3 = (a.width - 2 * gap) / 3, w2 = (a.width - gap) / 2;
    bool *field[5] = {&ui->cfg.use_keyboard, &ui->cfg.use_mouse, NULL, &ui->cfg.use_speaker, &ui->cfg.use_mic};
    for (int i = 0; i < 5; ++i) {
        Rectangle r = i < 3 ? (Rectangle){a.x + (float)i * (w3 + gap), a.y, w3, h}
                            : (Rectangle){a.x + (float)(i - 3) * (w2 + gap), a.y + h + gap, w2, h};
        if (!devices_tile(ui, r, &t[i])) {
            continue;
        }
        if (field[i]) {
            *field[i] = !*field[i];
            ui_save_config(ui); /* applied by the engine straight away */
        } else {
            devices_set_drive(ui, !ui->cfg.drive_enabled);
        }
    }
    float y = a.y + 2 * h + gap + 14;
    if (y + 24 < a.y + a.height) {
        ui_text(ui, "Keyboard and mouse switch at once. Turning sound or the drive on or off restarts the target Pico, "
                    "which takes a few seconds.", a.x, y, UI_FONT_SMALL, UI_MUTED);
    }
}
