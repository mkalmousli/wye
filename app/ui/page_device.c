/* Picos: both boards side by side - firmware, health, updates - plus recovery and new hardware. */
#include "device_pico.h"
#include "ui_fx.h"
#include "ui_theme.h"

static void legacy_card(struct wye_ui *ui, Rectangle r)
{
    ui_glow((Vector2){r.x + 40, r.y + r.height / 2}, 90, UI_WARN, 0.18f);
    Rectangle c = ui_card(ui, r, NULL);
    ui_text_bold(ui, "Old firmware found", c.x, c.y, UI_FONT, UI_WARN);
    ui_text(ui, TextFormat("The Pico on %s runs firmware Wye cannot talk to.", ui->st.link.legacy_port), c.x,
            c.y + 26, UI_FONT_SMALL, UI_MUTED);
    if (ui_button_tinted((Rectangle){c.x + c.width - 240, c.y + 4, 240, UI_ROW}, "Install current firmware",
                         UI_WARN)) {
        if (wye_engine_rescue_legacy()) {
            ui->setup_step = 2;
            ui->page = PAGE_SETUP;
        } else {
            ui_toast(ui, "Could not restart the Pico; use the setup wizard instead");
        }
    }
}

static void footer(struct wye_ui *ui, Rectangle r)
{
    Rectangle c = ui_card(ui, r, NULL);
    ui_text_bold(ui, "New Pico, or one that is not answering?", c.x, c.y, UI_FONT, UI_TEXT);
    ui_text(ui, "The setup guide installs firmware with the BOOTSEL button. After that, updates are automatic.", c.x,
            c.y + 26, UI_FONT_SMALL, UI_MUTED);
    if (ui_button_primary((Rectangle){c.x + c.width - 200, c.y + 4, 200, UI_ROW}, "Open setup guide")) {
        ui->setup_step = 0;
        ui->page = PAGE_SETUP;
    }
}

void page_device(struct wye_ui *ui, Rectangle a)
{
    const struct wye_link_info *l = &ui->st.link;
    ui_text_bold(ui, "Picos", a.x, a.y - 4, 30, UI_TEXT);
    ui_text(ui, "The two boards: their names, their firmware, and how healthy the link is.", a.x + 96, a.y + 8,
            UI_FONT_SMALL, UI_MUTED);
    float y = a.y + 56;
    if (l->state == WYE_LINK_LEGACY) {
        legacy_card(ui, (Rectangle){a.x, y, a.width, 92});
        y += 106;
    }
    struct device_pico picos[2] = {
        {"Bridge Pico", "Plugged into this computer", "Not found - plug it in", &l->bridge, WYE_ROLE_BRIDGE,
         WYE_NODE_BRIDGE, ui->st.bridge_outdated, ui_pico_color(ui, &l->bridge, UI_ACCENT)},
        {"Target Pico", "Plugged into the other computer", l->bridge.present ? "Not answering - check the wires" : "-",
         &l->target, WYE_ROLE_TARGET, WYE_NODE_TARGET, ui->st.target_outdated, ui_pico_color(ui, &l->target, UI_TARGET)},
    };
    bool wide = a.width >= 860;
    float w = wide ? (a.width - UI_PAD) / 2 : a.width, h = 236;
    for (int i = 0; i < 2; ++i) {
        Rectangle r = wide ? (Rectangle){a.x + (float)i * (w + UI_PAD), y, w, h}
                           : (Rectangle){a.x, y + (float)i * (h + UI_PAD), w, h};
        device_pico_card(ui, r, &picos[i]);
    }
    y += wide ? h + UI_PAD : 2 * (h + UI_PAD);
    if (ui->st.fw_phase == WYE_FWS_FAILED && ui->st.fw_error[0]) {
        ui_text(ui, ui->st.fw_error, a.x, y, UI_FONT_SMALL, UI_BAD);
        y += 28;
    }
    if (y + 96 + 112 <= a.y + a.height) {
        device_stats(ui, (Rectangle){a.x, y, a.width, 96});
        y += 96 + UI_PAD;
    }
    if (y + 84 <= a.y + a.height) {
        footer(ui, (Rectangle){a.x, y, a.width, 84});
    }
}
