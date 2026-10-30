/* USB identity: how a Pico introduces itself over USB - typed in, or copied from a real device. */
#include "ui.h"
#include "ui_theme.h"

#include "product.h"
#include "raygui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool identity_device_list(struct wye_ui *ui, Rectangle r, struct plat_usb_device *picked);
bool identity_apply(struct wye_ui *ui, int node, const char *vid, const char *pid, char fields[3][WYE_ID_STR_MAX]);

static struct {
    int node;                 /* WYE_NODE_BRIDGE / WYE_NODE_TARGET */
    bool loaded;
    char vid[8], pid[8], fields[3][WYE_ID_STR_MAX];
    int editing;              /* 0 none, 1..5 field */
    double requested;
} g = {.node = WYE_NODE_TARGET};

static void fill(uint16_t vid, uint16_t pid, const char *mfr, const char *product, const char *serial)
{
    snprintf(g.vid, sizeof(g.vid), "%04x", vid);
    snprintf(g.pid, sizeof(g.pid), "%04x", pid);
    snprintf(g.fields[0], WYE_ID_STR_MAX, "%s", mfr);
    snprintf(g.fields[1], WYE_ID_STR_MAX, "%s", product);
    snprintf(g.fields[2], WYE_ID_STR_MAX, "%s", serial);
    g.loaded = true;
}

static void field(struct wye_ui *ui, Rectangle r, const char *label, char *buf, int size, int index)
{
    ui_text(ui, label, r.x, r.y - 20, UI_FONT_SMALL, UI_MUTED);
    if (GuiTextBox(r, buf, size, g.editing == index)) {
        g.editing = g.editing == index ? 0 : index;
    }
}

static void editor(struct wye_ui *ui, Rectangle c)
{
    float y = c.y + 22, w = c.width;
    field(ui, (Rectangle){c.x, y, 90, UI_ROW}, "VID (hex)", g.vid, sizeof(g.vid), 1);
    field(ui, (Rectangle){c.x + 100, y, 90, UI_ROW}, "PID (hex)", g.pid, sizeof(g.pid), 2);
    field(ui, (Rectangle){c.x + 200, y, w - 200, UI_ROW}, "Serial number", g.fields[2], WYE_ID_STR_MAX, 5);
    y += UI_ROW + 30;
    field(ui, (Rectangle){c.x, y, w * 0.4f - 10, UI_ROW}, "Manufacturer", g.fields[0], WYE_ID_STR_MAX, 3);
    field(ui, (Rectangle){c.x + w * 0.4f, y, w * 0.6f, UI_ROW}, "Product name", g.fields[1], WYE_ID_STR_MAX, 4);
    y += UI_ROW + 14;
    if (ui_button_primary((Rectangle){c.x, y, 140, UI_ROW}, "Apply") && identity_apply(ui, g.node, g.vid, g.pid, g.fields)) {
        g.loaded = false;
    }
    if (ui_button((Rectangle){c.x + 150, y, 170, UI_ROW}, "Wye default")) {
        bool target = g.node == WYE_NODE_TARGET;
        fill(WYE_BRIDGE_VID, target ? WYE_TARGET_PID : WYE_BRIDGE_PID, WYE_VENDOR_NAME,
             target ? "Wye Keyboard and Mouse" : "Wye Bridge", target ? "WYE-TARGET" : "WYE-BRIDGE");
    }
    if (ui_button((Rectangle){c.x + 330, y, 120, UI_ROW}, "Revert")) {
        g.loaded = false;
    }
}

void identity_panel(struct wye_ui *ui, Rectangle a)
{
    int choice = g.node == WYE_NODE_TARGET ? 0 : 1, before = choice;
    choice = ui_segmented((Rectangle){a.x + a.width - 240, a.y, 240, UI_ROW}, "Target Pico;Bridge Pico", choice);
    if (choice != before) {
        g.node = choice ? WYE_NODE_BRIDGE : WYE_NODE_TARGET;
        g.loaded = false;
    }
    if (!g.loaded && ui->st.identity_valid[g.node]) {
        const struct wye_identity *id = &ui->st.identity[g.node];
        fill(id->vid, id->pid, id->manufacturer, id->product, id->serial);
    } else if (!g.loaded && GetTime() - g.requested > 2.0) {
        g.requested = GetTime();
        wye_engine_identity_request((uint8_t)g.node);
    }
    ui_text(ui, "One VID/PID for the whole Pico; each part gets its own name in its panel.", a.x, a.y + 10,
            UI_FONT_SMALL, UI_MUTED);
    Rectangle c = ui_card(ui, (Rectangle){a.x, a.y + 44, a.width, 210}, "How it shows up over USB");
    if (g.loaded) {
        editor(ui, c);
    } else {
        ui_text(ui, "Connect this Pico to see and change its identity.", c.x, c.y, UI_FONT, UI_MUTED);
    }
    struct plat_usb_device picked;
    if (identity_device_list(ui, (Rectangle){a.x, a.y + 266, a.width, a.height - 266}, &picked) && g.loaded) {
        fill(picked.vid, picked.pid, picked.manufacturer, picked.product, picked.serial);
        ui_toast(ui, "Copied - check it, then press Apply");
    }
}

void page_identity(struct wye_ui *ui, Rectangle a)
{
    ui_text_bold(ui, "USB identity", a.x, a.y, UI_FONT_TITLE, UI_TEXT);
    if (ui_button((Rectangle){a.x + a.width - 100, a.y - 2, 100, UI_ROW}, "Back")) {
        ui->page = PAGE_DEVICES;
    }
    identity_panel(ui, (Rectangle){a.x, a.y + 44, a.width - 110, a.height - 44});
}
