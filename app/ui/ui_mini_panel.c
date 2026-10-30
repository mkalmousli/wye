/* Mini mode strip (324x40): status on the left (drag handle), icon buttons on the right. */
#include "ui.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include <math.h>

void ui_mini_drag(struct wye_ui *ui, Rectangle grip);
void ui_mini_keep_on_all_desktops(struct wye_ui *ui);

static const char *chord(const struct wye_ui *ui, enum wye_input_action a)
{
    unsigned char key = ui->cfg.shortcuts[a];
    return key ? TextFormat("  %s+%s", ui_key_name(ui->cfg.hotkey), ui_key_name(key)) : "";
}

static Color drive_color(const struct wye_drive_status *d)
{
    if (!d->enabled || d->state == WYE_DRIVE_OFF) {
        return UI_MUTED;
    }
    return d->error ? UI_BAD : d->state == WYE_DRIVE_SYNCED ? UI_OK : UI_ACCENT;
}

/* Draws the buttons; returns the tooltip of the hovered one (or NULL). */
static const char *buttons(struct wye_ui *ui, float x)
{
    bool fwd = ui->st.mode != WYE_FWD_OFF, ready = ui->st.link.state == WYE_LINK_READY, hot = false;
    const char *tip = NULL;
    Rectangle r = {x, 6, 30, 28};
    if (ui_icon_button(r, UI_ICON_CONTROL, fwd ? UI_TARGET : UI_TEXT, fwd, ready || fwd, &hot)) {
        wye_engine_toggle();
    }
    tip = hot ? TextFormat(fwd ? "Return here  %s" : "Control target  %s", ui_key_name(ui->cfg.hotkey)) : tip;
    r.x += 32, hot = false;
    if (ui_icon_button(r, UI_ICON_JIGGLE, ui->st.jiggler_on ? UI_OK : UI_TEXT, ui->st.jiggler_on, true, &hot)) {
        wye_engine_jiggler(!ui->st.jiggler_on);
    }
    tip = hot ? TextFormat("Jiggler %s%s", ui->st.jiggler_on ? "on" : "off", chord(ui, WYE_ACT_JIGGLER)) : tip;
    r.x += 32, hot = false;
    if (ui_icon_button(r, UI_ICON_PASTE, UI_TEXT, false, ready, &hot)) {
        const char *clip = GetClipboardText();
        wye_engine_type_text(clip ? clip : "");
    }
    tip = hot ? TextFormat("Type clipboard%s", chord(ui, WYE_ACT_PASTE)) : tip;
    r.x += 32, hot = false;
    if (ui_icon_button(r, UI_ICON_FOLDER, drive_color(&ui->st.drive), false, true, &hot)) {
        plat_reveal_path(ui->st.drive.folder);
    }
    tip = hot ? TextFormat("Shared folder: %s", ui->st.drive.enabled ? ui->st.drive.message : "off") : tip;
    r.x += 36, hot = false;
    if (ui_icon_button((Rectangle){r.x, r.y, 24, 28}, UI_ICON_EXPAND, UI_MUTED, false, true, &hot)) {
        ui_mini_leave(ui);
    }
    tip = hot ? TextFormat("Full window%s", chord(ui, WYE_ACT_MINI)) : tip;
    r.x += 26, hot = false;
    if (ui_icon_button((Rectangle){r.x, r.y, 24, 28}, UI_ICON_CLOSE, UI_MUTED, false, true, &hot)) {
        ui->quit = true;
    }
    return hot ? "Close Wye" : tip;
}

void ui_mini_panel(struct wye_ui *ui)
{
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
    bool fwd = ui->st.mode != WYE_FWD_OFF;
    Color c;
    const char *status = ui_status_text(ui, &c);
    ui_mini_keep_on_all_desktops(ui);
    DrawRectangleRec((Rectangle){0, 0, w, h}, UI_CARD);
    DrawRectangleLinesEx((Rectangle){0, 0, w, h}, 1.5f, fwd ? UI_TARGET : UI_CARD_EDGE);
    for (int i = 0; i < 3; ++i) { /* grip dots: this part moves the window */
        DrawCircleV((Vector2){7, 13 + (float)i * 7}, 1.3f, UI_MUTED);
    }
    ui_dot(20, h / 2, c);
    const char *tip = buttons(ui, w - 200);
    const char *toast = ui->toast_count ? ui->toasts[ui->toast_count - 1].text : NULL;
    const char *line = tip ? tip : (toast && GetTime() < ui->toasts[ui->toast_count - 1].until) ? toast : status;
    /* Long lines scroll inside the narrow text area. */
    float area = w - 204 - 32, width = ui_measure(line, UI_FONT_SMALL, false).x;
    float shift = width > area ? fmodf((float)GetTime() * 40.0f, width + 40.0f) : 0;
    BeginScissorMode(32, 0, (int)area, (int)h);
    ui_text(ui, line, 32 - shift, h / 2 - 8, UI_FONT_SMALL, tip ? UI_TEXT : UI_MUTED);
    if (shift > 0) {
        ui_text(ui, line, 32 - shift + width + 40, h / 2 - 8, UI_FONT_SMALL, tip ? UI_TEXT : UI_MUTED);
    }
    EndScissorMode();
    ui_mini_drag(ui, (Rectangle){0, 0, w - 204, h});
}
