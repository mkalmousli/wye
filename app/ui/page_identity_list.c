/* USB identity: the devices plugged into this computer, each with a "Copy" button. */
#include "ui.h"
#include "ui_theme.h"

#include <string.h>

#define MAX_DEVICES 64
#define ROW_H 44.0f
#define REFRESH_S 2.0

static struct {
    struct plat_usb_device dev[MAX_DEVICES];
    int count, scroll;
    double scanned;
} g;

static void refresh(void)
{
    if (GetTime() - g.scanned > REFRESH_S) { /* follows plugging and unplugging */
        g.scanned = GetTime();
        g.count = plat_usb_list(g.dev, MAX_DEVICES);
    }
}

static bool row(struct wye_ui *ui, Rectangle r, const struct plat_usb_device *d)
{
    DrawRectangleRounded(r, 0.2f, 6, CheckCollisionPointRec(GetMousePosition(), r) ? UI_CARD_EDGE : UI_BG);
    const char *name = d->product[0] ? d->product : "Unnamed device";
    ui_text_bold(ui, name, r.x + 10, r.y + 4, UI_FONT_SMALL, UI_TEXT);
    ui_text(ui, TextFormat("%s   %04x:%04x   %s%s", d->manufacturer[0] ? d->manufacturer : "unknown maker", d->vid,
                           d->pid, d->serial[0] ? "serial " : "", d->serial),
            r.x + 10, r.y + 23, UI_FONT_SMALL, UI_MUTED);
    return ui_button((Rectangle){r.x + r.width - 96, r.y + 7, 86, 30}, "Copy");
}

/* True when the user picked a device; *picked then holds it. */
bool identity_device_list(struct wye_ui *ui, Rectangle area, struct plat_usb_device *picked)
{
    Rectangle c = ui_card(ui, area, "Copy from a device on this computer");
    ui_text(ui, "To look like a keyboard of the other computer, plug that keyboard in here for a moment and copy it.",
            c.x, c.y, UI_FONT_SMALL, UI_MUTED);
    refresh();
    float top = c.y + 26;
    int visible = (int)((c.y + c.height - top) / ROW_H);
    if (CheckCollisionPointRec(GetMousePosition(), c)) {
        g.scroll -= (int)GetMouseWheelMove();
    }
    int max_scroll = g.count - visible > 0 ? g.count - visible : 0;
    g.scroll = g.scroll < 0 ? 0 : (g.scroll > max_scroll ? max_scroll : g.scroll);
    bool chosen = false;
    for (int i = 0; i < visible && g.scroll + i < g.count; ++i) {
        const struct plat_usb_device *d = &g.dev[g.scroll + i];
        if (row(ui, (Rectangle){c.x, top + (float)i * ROW_H, c.width, ROW_H - 4}, d)) {
            *picked = *d;
            chosen = true;
        }
    }
    if (g.count == 0) {
        ui_text(ui, "No USB devices found.", c.x, top, UI_FONT, UI_MUTED);
    } else if (max_scroll > 0) {
        ui_text(ui, TextFormat("%d more - scroll", max_scroll - g.scroll), c.x + c.width - 120,
                c.y, UI_FONT_SMALL, UI_ACCENT);
    }
    return chosen;
}
