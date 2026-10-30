/*
 * Picos page: one Pico as a card - its picture with a live LED, the firmware
 * it runs against the one built in, what it can do, and update / progress.
 */
#include "device_pico.h"
#include "setup_flash.h"
#include "ui_fx.h"
#include "ui_theme.h"

#include "fw/fw_image.h"
#include "raygui.h"

static void features(struct wye_ui *ui, float x, float y, uint32_t caps)
{
    static const struct { uint32_t cap; const char *name; } F[] = {
        {WYE_CAP_FW_UPDATE, "Updates"}, {WYE_CAP_IDENTITY, "USB identity"},
        {WYE_CAP_DISK, "Shared folder"}, {WYE_CAP_AUDIO, "Audio"},
    };
    for (size_t i = 0; i < sizeof(F) / sizeof(F[0]); ++i) {
        if (caps & F[i].cap) {
            x += ui_chip(ui, x, y, F[i].name, UI_ACCENT2) + 6;
        }
    }
}

/* A long name gets smaller before it gets cut off. */
static void title(struct wye_ui *ui, const char *name, Rectangle r)
{
    float size = 22;
    while (size > 15 && ui_measure(name, size, true).x > r.width) {
        size -= 2;
    }
    BeginScissorMode((int)r.x, (int)r.y, (int)r.width, (int)r.height);
    ui_text_bold(ui, name, r.x, r.y + (22 - size) / 2, size, UI_TEXT);
    EndScissorMode();
}

/* The update area: progress while this Pico is being written, else the button. */
static void action(struct wye_ui *ui, Rectangle r, const struct device_pico *p, const struct wye_fw_image *img)
{
    bool mine = ui->st.fw_phase != WYE_FWS_IDLE && ui->st.fw_node == p->node;
    if (mine) {
        const char *phase = ui->st.fw_phase == WYE_FWS_REBOOT ? "Restarting with the new firmware..."
                          : ui->st.fw_phase == WYE_FWS_FAILED ? "Update failed - try again"
                          : TextFormat("Installing  %d%%", ui->st.fw_progress);
        ui_text_bold(ui, phase, r.x, r.y, UI_FONT, p->tint);
        ui_progress((Rectangle){r.x, r.y + 28, r.width, 8}, ui->st.fw_progress, p->tint);
        return;
    }
    bool busy = ui->st.fw_phase != WYE_FWS_IDLE || ui->st.mode != WYE_FWD_OFF || !img || !p->info->present;
    if (busy) {
        GuiDisable();
    }
    bool pressed = p->outdated && img
                       ? ui_button_primary(r, TextFormat("Update to %u.%u.%u", img->major, img->minor, img->patch))
                       : ui_button(r, "Reinstall firmware");
    if (pressed && !wye_engine_update_firmware(p->role)) {
        ui_toast(ui, "Firmware update could not start");
    }
    GuiEnable();
}

void device_pico_card(struct wye_ui *ui, Rectangle r, const struct device_pico *p)
{
    Rectangle c = ui_card(ui, r, NULL);
    const struct wye_node_info *n = p->info;
    const struct wye_hello_ack *h = &n->hello;
    bool writing = ui->st.fw_phase != WYE_FWS_IDLE && ui->st.fw_node == p->node;
    Color led = !n->present ? BLANK : writing ? Fade(p->tint, ui_pulse(3.0f)) : UI_OK;
    Rectangle board = {c.x, c.y + 4, 150, 60};
    if (n->present) {
        ui_glow((Vector2){board.x + 75, board.y + 30}, 90, p->tint, writing ? 0.3f : 0.14f);
    }
    art_pico(ui, board, p->role, led);
    if (device_pico_editing(p->node)) {
        device_pico_editor(ui, c, p);
        return;
    }
    float x = c.x + 170;
    const char *name = ui_pico_name(ui, p->node, p->title);
    title(ui, name, (Rectangle){x, c.y, c.width - 170, 30});
    ui_text(ui, p->where, x, c.y + 32, UI_FONT_SMALL, UI_MUTED);
    const char *notes = ui->st.profile_valid[p->node] ? ui->st.profile[p->node].notes : "";
    if (name != p->title || notes[0]) { /* still say which board it is */
        const char *line = name == p->title ? notes : notes[0] ? TextFormat("%s - %s", p->title, notes) : p->title;
        BeginScissorMode((int)x, (int)c.y + 48, (int)(c.width - 170), 22);
        ui_text(ui, line, x, c.y + 50, UI_FONT_SMALL, UI_MUTED);
        EndScissorMode();
    }
    float y = c.y + 86;
    const struct wye_fw_image *img = wye_fw_embedded(p->role);
    if (!n->present) {
        ui_chip(ui, c.x, y, p->missing, UI_MUTED);
    } else {
        const char *ver = TextFormat("%u.%u.%u", h->fw_major, h->fw_minor, h->fw_patch);
        float w = ui_chip(ui, c.x, y, p->outdated && img ? TextFormat("%s  >  %u.%u.%u available", ver, img->major,
                                                                     img->minor, img->patch)
                                                         : TextFormat("Firmware %s - up to date", ver),
                          p->outdated ? UI_WARN : UI_OK);
        ui_text(ui, TextFormat("%u ms   %u errors", n->rtt_ms, n->pong.rx_errors), c.x + w + 12, y + 5,
                UI_FONT_SMALL, UI_MUTED);
        features(ui, c.x, y + 36, h->caps);
    }
    float bw = 120;
    action(ui, (Rectangle){c.x, c.y + c.height - UI_ROW, c.width - bw - 8, UI_ROW}, p, img);
    if (!n->present) {
        GuiDisable();
    }
    if (ui_button((Rectangle){c.x + c.width - bw, c.y + c.height - UI_ROW, bw, UI_ROW}, "Personalize")) {
        if (ui->st.profile_valid[p->node]) {
            device_pico_edit_open(ui, p);
        } else {
            ui_toast(ui, "Update this Pico's firmware first: it stores the name");
        }
    }
    GuiEnable();
}
