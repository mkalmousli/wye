/* Your own name and colour for a Pico: lookups for the pages, and the editor on its card. */
#include "device_pico.h"
#include "ui_theme.h"

#include <stdio.h>
#include <string.h>

static const Color COLORS[WYE_PICO_COLORS] = {
    {0, 0, 0, 0}, /* the role's own */
    {79, 140, 255, 255}, {168, 85, 247, 255}, {34, 211, 238, 255},
    {52, 211, 153, 255}, {251, 191, 36, 255}, {236, 72, 153, 255},
};

static struct {
    int node;       /* the Pico being edited; 0: none */
    int color, editing;
    char name[WYE_PICO_NAME_MAX * 2 + 1];
} g;

static const struct wye_pico_name *entry(const struct wye_ui *ui, const struct wye_node_info *n)
{
    return n->present ? wye_pico_names_find(&ui->pico_names, n->hello.uid) : NULL;
}

const char *ui_pico_name(const struct wye_ui *ui, const struct wye_node_info *n, const char *fallback)
{
    const struct wye_pico_name *e = entry(ui, n);
    return e && e->name[0] ? e->name : fallback;
}

Color ui_pico_color(const struct wye_ui *ui, const struct wye_node_info *n, Color fallback)
{
    const struct wye_pico_name *e = entry(ui, n);
    return e && e->color ? COLORS[e->color] : fallback;
}

bool device_pico_editing(int node)
{
    return g.node == node;
}

void device_pico_edit_open(const struct wye_ui *ui, const struct device_pico *p)
{
    const struct wye_pico_name *e = entry(ui, p->info);
    g.node = p->node;
    g.color = e ? e->color : 0;
    g.editing = 1;
    snprintf(g.name, sizeof(g.name), "%s", e ? e->name : "");
}

static void save(struct wye_ui *ui, const struct device_pico *p, const char *name, int color)
{
    wye_pico_names_set(&ui->pico_names, p->info->hello.uid, name, color);
    if (!wye_pico_names_save(&ui->pico_names)) {
        ui_toast(ui, "Could not save the name");
    }
    g.node = 0;
}

void device_pico_editor(struct wye_ui *ui, Rectangle c, const struct device_pico *p)
{
    if (!p->info->present) {
        g.node = 0; /* it was unplugged */
        return;
    }
    float x = c.x + 170;
    bool enter = g.editing && IsKeyPressed(KEY_ENTER); /* before the box, which ends editing on it */
    ui_textbox(ui, (Rectangle){x, c.y + 22, c.width - 170, UI_ROW}, "Name", g.name, WYE_PICO_NAME_MAX + 1, 1, &g.editing);
    if (!g.name[0] && !g.editing) {
        ui_text(ui, p->title, x + 10, c.y + 31, UI_FONT_SMALL, UI_MUTED);
    }
    ui_text(ui, "Colour", c.x, c.y + 90, UI_FONT_SMALL, UI_MUTED);
    for (int i = 0; i < WYE_PICO_COLORS; ++i) {
        Vector2 at = {c.x + 70 + (float)i * 34, c.y + 98};
        Color col = i ? COLORS[i] : p->role == WYE_ROLE_TARGET ? UI_TARGET : UI_ACCENT;
        if (i == g.color) {
            DrawRing(at, 12, 15, 0, 360, 32, UI_TEXT);
        }
        DrawCircleV(at, 10, col);
        if (!i) {
            DrawCircleV(at, 4, UI_CARD); /* "the usual one" */
        }
        if (CheckCollisionPointCircle(GetMousePosition(), at, 14) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g.color = i;
        }
    }
    float w = (c.width - 16) / 3, y = c.y + c.height - UI_ROW;
    if (ui_button_primary((Rectangle){c.x, y, w, UI_ROW}, "Save") || enter) {
        save(ui, p, g.name, g.color);
        return;
    }
    if (ui_button((Rectangle){c.x + w + 8, y, w, UI_ROW}, "Cancel") || IsKeyPressed(KEY_ESCAPE)) {
        g.node = 0;
        return;
    }
    if (ui_button((Rectangle){c.x + 2 * (w + 8), y, w, UI_ROW}, "Use default")) {
        save(ui, p, "", 0);
    }
}
