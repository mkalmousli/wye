/* The name, colour and notes stored on a Pico: lookups for the pages, and the editor on its card. */
#include "device_pico.h"
#include "ui_theme.h"

#include "raygui.h"

#include <stdio.h>
#include <string.h>

#define CONFIRM_S 3.0

static const Color COLORS[WYE_PROFILE_COLORS] = {
    {0, 0, 0, 0}, /* the role's own */
    {79, 140, 255, 255}, {168, 85, 247, 255}, {34, 211, 238, 255},
    {52, 211, 153, 255}, {251, 191, 36, 255}, {236, 72, 153, 255},
};

static struct {
    int node;       /* the Pico being edited; 0: none */
    int editing;
    struct wye_profile f;
    struct wye_profile sent;
    double wait_until;  /* waiting for the Pico to confirm what was sent; 0: not */
} g;

static const struct wye_profile *stored(const struct wye_ui *ui, int node)
{
    return ui->st.profile_valid[node] ? &ui->st.profile[node] : NULL;
}

const char *ui_pico_name(const struct wye_ui *ui, int node, const char *fallback)
{
    const struct wye_profile *f = stored(ui, node);
    return f && f->name[0] ? f->name : fallback;
}

Color ui_pico_color(const struct wye_ui *ui, int node, Color fallback)
{
    const struct wye_profile *f = stored(ui, node);
    return f && f->color ? COLORS[f->color] : fallback;
}

bool device_pico_editing(int node)
{
    return g.node == node;
}

void device_pico_edit_open(const struct wye_ui *ui, const struct device_pico *p)
{
    g.node = p->node;
    g.f = ui->st.profile[p->node];
    g.editing = 1;
    g.wait_until = 0;
}

static bool same(const struct wye_profile *a, const struct wye_profile *b)
{
    return a->color == b->color && strcmp(a->name, b->name) == 0 && strcmp(a->notes, b->notes) == 0;
}

static void save(struct wye_ui *ui, const struct device_pico *p, const struct wye_profile *f)
{
    g.sent = *f;
    wye_profile_clean(g.sent.name, WYE_PROFILE_NAME_MAX);
    wye_profile_clean(g.sent.notes, WYE_PROFILE_NOTES_MAX);
    if (same(&g.sent, &ui->st.profile[p->node])) {
        g.node = 0; /* nothing changed: spare the flash */
    } else if (wye_engine_profile_set((uint8_t)p->node, &g.sent)) {
        g.wait_until = GetTime() + CONFIRM_S;
    } else {
        ui_toast(ui, "Could not reach this Pico");
    }
}

/* Waiting for the board to answer with what was sent. */
static void check_confirmed(struct wye_ui *ui, const struct device_pico *p)
{
    if (!g.wait_until) {
        return;
    }
    if (ui->st.profile_valid[p->node] && same(&g.sent, &ui->st.profile[p->node])) {
        ui_toast(ui, "Saved on the Pico");
        g.node = 0;
    } else if (GetTime() > g.wait_until) {
        ui_toast(ui, "The Pico did not confirm. Try again.");
        g.wait_until = 0;
    }
}

static void colors(float x, float y, const struct device_pico *p)
{
    for (int i = 0; i < (int)WYE_PROFILE_COLORS; ++i) {
        Vector2 at = {x + (float)i * 34, y};
        Color col = i ? COLORS[i] : p->role == WYE_ROLE_TARGET ? UI_TARGET : UI_ACCENT;
        if (i == g.f.color) {
            DrawRing(at, 12, 15, 0, 360, 32, UI_TEXT);
        }
        DrawCircleV(at, 10, col);
        if (!i) {
            DrawCircleV(at, 4, UI_CARD); /* "the usual one" */
        }
        if (CheckCollisionPointCircle(GetMousePosition(), at, 14) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g.f.color = (uint8_t)i;
        }
    }
}

void device_pico_editor(struct wye_ui *ui, Rectangle c, const struct device_pico *p)
{
    if (!p->info->present) {
        g.node = 0; /* it was unplugged */
        return;
    }
    check_confirmed(ui, p);
    if (!g.node) {
        return;
    }
    float x = c.x + 170;
    bool enter = g.editing && IsKeyPressed(KEY_ENTER); /* before the boxes, which end editing on it */
    ui_textbox(ui, (Rectangle){x, c.y + 22, c.width - 170, UI_ROW}, "Name", g.f.name, WYE_PROFILE_NAME_MAX + 1, 1, &g.editing);
    if (!g.f.name[0] && g.editing != 1) {
        ui_text(ui, p->title, x + 10, c.y + 31, UI_FONT_SMALL, UI_MUTED);
    }
    ui_text(ui, "Colour", c.x, c.y + 72, UI_FONT_SMALL, UI_MUTED);
    colors(c.x + 70, c.y + 80, p);
    ui_text(ui, "Notes", c.x, c.y + 117, UI_FONT_SMALL, UI_MUTED);
    ui_textbox(ui, (Rectangle){c.x + 58, c.y + 108, c.width - 58, UI_ROW}, NULL, g.f.notes, WYE_PROFILE_NOTES_MAX + 1, 2, &g.editing);
    float w = (c.width - 16) / 3, y = c.y + c.height - UI_ROW;
    bool waiting = g.wait_until != 0, controlling = ui->st.mode != WYE_FWD_OFF;
    if (waiting || controlling) {
        GuiDisable(); /* writing flash pauses the Pico for a moment: not while keys are crossing */
    }
    bool pressed = ui_button_primary((Rectangle){c.x, y, w, UI_ROW}, waiting ? "Saving..." : "Save on Pico");
    bool reset = ui_button((Rectangle){c.x + 2 * (w + 8), y, w, UI_ROW}, "Use default");
    GuiEnable();
    if (!waiting && !controlling && (pressed || enter)) {
        save(ui, p, &g.f);
    } else if (!waiting && !controlling && reset) {
        static const struct wye_profile BLANK_PROFILE;
        save(ui, p, &BLANK_PROFILE);
    }
    if (ui_button((Rectangle){c.x + w + 8, y, w, UI_ROW}, "Cancel") || IsKeyPressed(KEY_ESCAPE)) {
        g.node = 0;
    }
}
