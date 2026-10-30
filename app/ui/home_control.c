/*
 * Home: the control switch. A big glowing key with the hotkey on it: blue
 * when ready, pulsing purple while this keyboard and mouse drive the other
 * computer, grey (with the reason) when it cannot be used.
 */
#include "ui.h"
#include "ui_fx.h"
#include "ui_theme.h"
#include <math.h>

static const char *blocker(const struct wye_ui *ui)
{
    if (ui->st.link.state != WYE_LINK_READY) {
        return ui->st.link.state == WYE_LINK_BRIDGE ? "The other computer's Pico is not answering"
                                                    : "Plug in the bridge Pico to start";
    }
    return ui->st.capture == PLAT_INPUT_RUNNING ? NULL : "Wye cannot read your keyboard yet";
}

static void words(struct wye_ui *ui, Rectangle r, bool fwd, const char *why)
{
    const char *key = ui_key_name(ui->cfg.hotkey);
    const char *title = fwd ? "You are in control" : why ? "Not ready" : "Take control";
    const char *line1 = fwd ? "Your keyboard and mouse now drive" : why ? why : "Use this keyboard and mouse";
    const char *line2 = fwd ? TextFormat("the other computer. %s brings them back.", key)
                      : why ? "" : TextFormat("on the other computer. Tap %s or click.", key);
    ui_text_bold(ui, title, r.x, r.y, 24, fwd ? UI_TARGET : why ? UI_MUTED : UI_TEXT);
    ui_text(ui, line1, r.x, r.y + 36, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, line2, r.x, r.y + 56, UI_FONT_SMALL, UI_MUTED);
}

void home_control(struct wye_ui *ui, Rectangle card)
{
    Rectangle c = ui_card(ui, card, NULL);
    bool fwd = ui->st.mode != WYE_FWD_OFF;
    const char *why = fwd ? NULL : blocker(ui);
    float radius = fminf(c.height * 0.42f, 64);
    Vector2 at = {c.x + radius + 6, c.y + c.height / 2};
    bool hot = !why && CheckCollisionPointCircle(GetMousePosition(), at, radius + 6);
    float lift = ui_tween(ui_id("control-key", card.x, card.y), hot ? 1.0f : 0.0f, 12);
    float active = ui_tween(ui_id("control-on", card.x, card.y), fwd ? 1.0f : 0.0f, 6);
    Color ring = why ? UI_CARD_EDGE : ui_mix(UI_ACCENT, UI_TARGET, active);
    float beat = fwd ? ui_pulse(0.9f) : 0.5f + 0.5f * lift;
    if (!why) {
        ui_glow(at, radius * (1.7f + 0.25f * beat), ring, 0.18f + 0.22f * beat);
    }
    DrawCircleV(at, radius, UI_CARD_HI);
    float sweep = fwd ? fmodf((float)GetTime() * 120, 360) : 0;
    DrawRing(at, radius - 6, radius, sweep, sweep + 360, 64, ring);
    if (fwd) { /* a bright arc running around while in control */
        DrawRing(at, radius - 6, radius, sweep, sweep + 90, 32, UI_TARGET2);
    }
    float kw = radius * 1.05f, kh = radius * 0.78f;
    ui_keycap(ui, (Rectangle){at.x - kw / 2, at.y - kh / 2 - lift * 3, kw, kh}, ui_key_name(ui->cfg.hotkey),
              !why, fwd ? UI_TARGET : UI_ACCENT);
    if (hot && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        wye_engine_toggle();
    }
    words(ui, (Rectangle){at.x + radius + 22, c.y + c.height / 2 - 40, c.width, 80}, fwd, why);
}
