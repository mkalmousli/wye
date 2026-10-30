/* Home: quick actions as tiles - each shows its state, its shortcut and reacts when hovered. */
#include "ui.h"
#include "ui_fx.h"
#include "ui_icons.h"
#include "ui_theme.h"

#include "raygui.h"

#include <math.h>

struct tile {
    enum ui_icon icon;
    const char *title, *state;
    enum wye_input_action shortcut;
    bool on, enabled;
    Color tint;
    float progress;              /* ring around the icon, 0 = none */
};

static bool draw(struct wye_ui *ui, Rectangle r, const struct tile *t)
{
    bool hot = t->enabled && CheckCollisionPointRec(GetMousePosition(), r);
    float lift = ui_tween(ui_id(t->title, r.x, 0), hot ? 1.0f : 0.0f, 12);
    Rectangle b = {r.x, r.y - 3 * lift, r.width, r.height};
    if (t->on || lift > 0.01f) {
        ui_glow((Vector2){b.x + 34, b.y + b.height / 2}, 70, t->tint, 0.12f * lift + (t->on ? 0.2f : 0));
    }
    ui_card(ui, b, NULL);
    DrawRectangleRoundedLinesEx(b, 16.0f / fminf(b.width, b.height), 10, 1.2f,
                                Fade(t->tint, (t->on ? 0.8f : 0) + 0.6f * lift));
    Vector2 ic = {b.x + 34, b.y + b.height / 2};
    DrawCircleV(ic, 20, Fade(t->tint, t->on ? 0.3f : 0.14f));
    if (t->progress > 0) {
        DrawRing(ic, 20, 22.5f, -90, -90 + 360 * t->progress, 36, t->tint);
    }
    ui_icon(t->icon, ic, t->enabled ? (t->on ? WHITE : t->tint) : UI_MUTED, t->on);
    ui_text_bold(ui, t->title, b.x + 64, b.y + 18, UI_FONT, t->enabled ? UI_TEXT : UI_MUTED);
    BeginScissorMode((int)b.x + 64, (int)b.y, (int)b.width - 70, (int)b.height);
    ui_text(ui, t->state, b.x + 64, b.y + 40, 12.5f, t->on ? t->tint : UI_MUTED);
    unsigned char key = ui->cfg.shortcuts[t->shortcut];
    if (key) {
        ui_text(ui, TextFormat("%s + %s", ui_key_name(ui->cfg.hotkey), ui_key_name(key)), b.x + 64, b.y + 58, 11.5f,
                Fade(UI_MUTED, 0.8f));
    }
    EndScissorMode();
    return hot && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

void home_actions(struct wye_ui *ui, Rectangle r)
{
    bool ready = ui->st.link.state == WYE_LINK_READY;
    const struct wye_drive_status *d = &ui->st.drive;
    struct tile tiles[4] = {
        {UI_ICON_JIGGLE, "Jiggler", ui->st.jiggler_on ? "On - keeps it awake" : "Off", WYE_ACT_JIGGLER,
         ui->st.jiggler_on, true, UI_OK, ui->st.jiggler_on ? (float)ui->st.jiggler_progress / 100.0f : 0},
        {UI_ICON_PASTE, "Type clipboard", ui->st.typing ? "Typing..." : "Types what you copied", WYE_ACT_PASTE,
         ui->st.typing, ready, UI_ACCENT, 0},
        {UI_ICON_BOLT, "Ctrl+Alt+Del", "Sends the secure keys", WYE_ACT_CTRL_ALT_DEL, false, ready, UI_WARN, 0},
        {UI_ICON_FOLDER, "Shared folder", d->enabled ? d->message : "Off", WYE_ACT_FOLDER,
         d->enabled && d->state == WYE_DRIVE_SYNCED, true, d->error ? UI_BAD : UI_ACCENT2, 0},
    };
    float gap = 12, w = (r.width - 3 * gap) / 4;
    for (int i = 0; i < 4; ++i) {
        if (!draw(ui, (Rectangle){r.x + (float)i * (w + gap), r.y + 3, w, r.height - 3}, &tiles[i])) {
            continue;
        }
        if (i == 0) {
            wye_engine_jiggler(!ui->st.jiggler_on);
        } else if (i == 1) {
            const char *clip = GetClipboardText();
            wye_engine_type_text(clip ? clip : "");
        } else if (i == 2) {
            wye_engine_ctrl_alt_del();
        } else {
            ui_open_shared_folder(ui);
        }
    }
}
