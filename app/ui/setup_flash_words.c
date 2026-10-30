/* Setup: what the install screen says (and offers) in each phase. */
#include "setup_flash.h"
#include "ui_theme.h"

#define TIPS_AFTER_S 12.0

struct words { const char *title, *line1, *line2; Color color; const char *button; enum flash_action act; };

#if defined(_WIN32)
#define STUCK_HINT "Wait a moment for Windows to finish, or unplug it and try again."
#else
#define STUCK_HINT "Open your file manager and click the RPI-RP2 drive once - Wye does the rest."
#endif

static const char *next_hint(const struct flash_state *s)
{
    if (flash_both_done(s)) {
        return "Both Picos are ready - click Next.";
    }
    return "Next: hold BOOTSEL on your other Pico and plug it in (the same port is fine).";
}

static struct words pick(const struct flash_state *s)
{
    bool none = !s->done[WYE_ROLE_BRIDGE] && !s->done[WYE_ROLE_TARGET];
    switch (s->phase) {
    case FLASH_OPENING:
        return (struct words){"Found a Pico!", "Opening it...", "", UI_ACCENT, NULL, 0};
    case FLASH_STUCK:
        return (struct words){"Your Pico is here, but its drive did not open", STUCK_HINT, "", UI_WARN,
                              "Try again", FLASH_ACT_RETRY};
    case FLASH_INSTALLING:
        return (struct words){s->role == WYE_ROLE_BRIDGE ? "Installing the bridge firmware..."
                                                         : "Installing the target firmware...",
                              "Keep it plugged in - this takes a few seconds.", "", UI_ACCENT, NULL, 0};
    case FLASH_DONE:
        return s->role == WYE_ROLE_BRIDGE
                   ? (struct words){"Bridge ready!", "This Pico belongs at this computer.", next_hint(s), UI_OK, NULL, 0}
                   : (struct words){"Target ready!", "This Pico goes to the other computer - unplug it.",
                                    next_hint(s), UI_OK, NULL, 0};
    case FLASH_ALL_SET:
        return (struct words){"Both Picos are ready", "Nothing to install - click Next.", "", UI_OK,
                              "Install again", FLASH_ACT_REINSTALL};
    case FLASH_UPDATING:
        return s->role == WYE_ROLE_BRIDGE
                   ? (struct words){"Updating the bridge - no button needed", "Sent over USB. Keep it plugged in.",
                                    "", UI_ACCENT, NULL, 0}
                   : (struct words){"Updating the target through the bridge",
                                    "Sent over the wires to the other Pico. Keep both plugged in.", "", UI_ACCENT,
                                    NULL, 0};
    case FLASH_RESCUE:
        return (struct words){"Found a Pico with old firmware", "Switching it to install mode - no button needed.",
                              "", UI_ACCENT, NULL, 0};
    case FLASH_FAILED:
        return (struct words){"That did not work", s->error, "", UI_BAD, "Try again", FLASH_ACT_RETRY};
    default:
        return (struct words){"Hold the white BOOTSEL button and plug a Pico in",
                              none ? "Either Pico is fine - Wye asks which one it is before installing."
                                   : next_hint(s),
                              "Let go once it is plugged in. A brand-new Pico does not even need the button.",
                              UI_TEXT, NULL, 0};
    }
}

void flash_ask(struct wye_ui *ui, const struct flash_state *s, Rectangle r, enum flash_action *action);

void flash_words(struct wye_ui *ui, const struct flash_state *s, Rectangle r, enum flash_action *action)
{
    if (s->phase == FLASH_ASK) {
        flash_ask(ui, s, r, action);
        return;
    }
    struct words w = pick(s);
    float text_w = r.width - (w.button ? 200 : 0);
    float size = ui_measure(w.title, 24, true).x > text_w ? 20 : 24;
    ui_text_bold(ui, w.title, r.x, r.y, size, w.color);
    ui_text(ui, w.line1, r.x, r.y + 36, UI_FONT, UI_TEXT);
    ui_text(ui, w.line2, r.x, r.y + 60, UI_FONT, UI_MUTED);
    if (s->phase == FLASH_UPDATING) {
        ui_progress((Rectangle){r.x, r.y + 68, text_w * 0.6f, 8}, ui->st.fw_progress, UI_ACCENT);
        ui_text(ui, TextFormat("%d%%", ui->st.fw_progress), r.x + text_w * 0.6f + 12, r.y + 62, UI_FONT, UI_MUTED);
    }
    if (s->phase == FLASH_WAITING && GetTime() - s->since > TIPS_AFTER_S) {
        ui_text(ui, "Nothing happening? Use a USB cable that carries data (some only charge), "
                    "or try another USB port.", r.x, r.y + 86, UI_FONT_SMALL, UI_WARN);
    }
    if (w.button && ui_button((Rectangle){r.x + r.width - 180, r.y, 180, UI_ROW}, w.button)) {
        *action = w.act;
    }
}
