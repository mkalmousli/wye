/* Picos page: the link in numbers - big, live figures under the two boards. */
#include "device_pico.h"
#include "ui_fx.h"
#include "ui_theme.h"

static const char *uptime(uint32_t ms)
{
    uint32_t s = ms / 1000;
    return s < 120 ? TextFormat("%us", s) : s < 7200 ? TextFormat("%um", s / 60) : TextFormat("%uh", s / 3600);
}

static void stat(struct wye_ui *ui, Rectangle r, const char *value, const char *label, Color tint)
{
    Rectangle c = ui_card(ui, r, NULL);
    ui_text_bold(ui, value, c.x, c.y - 4, 34, tint);
    ui_text(ui, label, c.x, c.y + 40, UI_FONT_SMALL, UI_MUTED);
}

void device_stats(struct wye_ui *ui, Rectangle r)
{
    const struct wye_link_info *l = &ui->st.link;
    bool t = l->target.present, b = l->bridge.present;
    unsigned errors = l->rx_errors + l->bridge.pong.rx_errors + l->target.pong.rx_errors;
    float gap = UI_PAD, w = (r.width - 3 * gap) / 4;
    stat(ui, (Rectangle){r.x, r.y, w, r.height}, t ? TextFormat("%u ms", l->target.rtt_ms) : "-",
         "to the other computer and back", t && l->target.rtt_ms < 20 ? UI_OK : UI_MUTED);
    stat(ui, (Rectangle){r.x + w + gap, r.y, w, r.height}, b ? TextFormat("%u", errors) : "-",
         "damaged messages (resent)", errors ? UI_WARN : b ? UI_OK : UI_MUTED);
    stat(ui, (Rectangle){r.x + 2 * (w + gap), r.y, w, r.height}, b ? uptime(l->bridge.pong.uptime_ms) : "-",
         "bridge running", b ? UI_ACCENT : UI_MUTED);
    stat(ui, (Rectangle){r.x + 3 * (w + gap), r.y, w, r.height}, t ? uptime(l->target.pong.uptime_ms) : "-",
         "target running", t ? UI_TARGET : UI_MUTED);
}
