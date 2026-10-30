/* Home: the live dashboard - link map, control switch, activity, quick actions. */
#include "ui.h"
#include "ui_fx.h"
#include "ui_theme.h"

bool home_banner(struct wye_ui *ui, Rectangle r);
void home_map(struct wye_ui *ui, Rectangle r);
void home_control(struct wye_ui *ui, Rectangle r);
void home_activity(struct wye_ui *ui, Rectangle r);
void home_actions(struct wye_ui *ui, Rectangle r);

static void chip(struct wye_ui *ui, float *right, float y, const char *text, Color col)
{
    float w = ui_chip_width(ui, text);
    ui_chip(ui, *right - w, y, text, col);
    *right -= w + 8;
}

static void header(struct wye_ui *ui, Rectangle a)
{
    Color c;
    const char *status = ui_status_text(ui, &c);
    ui_text_bold(ui, "Wye", a.x, a.y - 4, 30, UI_TEXT);
    float title = ui_measure("Wye", 30, true).x;
    ui_dot(a.x + title + 16, a.y + 15, c);
    ui_text(ui, status, a.x + title + 30, a.y + 6, UI_FONT, c);
    float right = a.x + a.width;
    const struct wye_link_info *l = &ui->st.link;
    if (l->target.present) {
        chip(ui, &right, a.y + 2, TextFormat("%u ms", l->target.rtt_ms), l->target.rtt_ms < 20 ? UI_OK : UI_WARN);
    }
}

void page_home(struct wye_ui *ui, Rectangle a)
{
    header(ui, a);
    float y = a.y + 48, gap = 14, bottom = a.y + a.height;
    if (home_banner(ui, (Rectangle){a.x, y, a.width, 46})) {
        y += 46 + gap;
    }
    float actions_h = 96, rest = bottom - y - actions_h - 2 * gap;
    float map_h = rest * 0.4f < 170 ? 170 : rest * 0.4f > 200 ? 200 : rest * 0.4f, row_h = rest - map_h - gap;
    home_map(ui, (Rectangle){a.x, y, a.width, map_h});
    y += map_h + gap;
    float control_w = a.width * 0.42f;
    home_control(ui, (Rectangle){a.x, y, control_w, row_h + gap});
    home_activity(ui, (Rectangle){a.x + control_w + gap, y, a.width - control_w - gap, row_h + gap});
    home_actions(ui, (Rectangle){a.x, bottom - actions_h, a.width, actions_h});
}
