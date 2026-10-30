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
    float x = c.x + 170;
    ui_text_bold(ui, p->title, x, c.y, 22, UI_TEXT);
    ui_text(ui, p->where, x, c.y + 32, UI_FONT_SMALL, UI_MUTED);
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
    action(ui, (Rectangle){c.x, c.y + c.height - UI_ROW, c.width, UI_ROW}, p, img);
}
