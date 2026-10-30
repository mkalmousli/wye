/*
 * Home: live activity. While controlling, the keys you press fly in as
 * keycaps and fade, a pad shows where the mouse is heading; sound meters and a
 * graph of the traffic to the other computer are always live.
 */
#include "ui.h"
#include "ui_fx.h"
#include "ui_theme.h"
#include "plat_thread.h"
#include <math.h>

#define HISTORY 90
#define KEY_LIFE_MS 2600.0f

void activity_pad(Rectangle r, float vx, float vy);
void activity_graph(Rectangle r, const float *rate, int count, int head);
void activity_sound(struct wye_ui *ui, Rectangle r);

static float g_rate[HISTORY], g_vx, g_vy;
static int g_head;

static void sample(const struct wye_ui *ui)
{
    static uint32_t last;
    static double next;
    float dt = GetFrameTime() > 0 ? GetFrameTime() : 0.016f;
    g_vx += ((float)ui->st.activity.dx / dt * 0.01f - g_vx) * fminf(1, dt * 10); /* motion, eased */
    g_vy += ((float)ui->st.activity.dy / dt * 0.01f - g_vy) * fminf(1, dt * 10);
    if (GetTime() >= next) {
        next = GetTime() + 0.1;
        g_rate[g_head] = (float)(ui->st.activity.messages - last) * 10.0f;
        g_head = (g_head + 1) % HISTORY;
        last = ui->st.activity.messages;
    }
}

/* Oldest on the left, newest on the right - like typing. */
static void keys(struct wye_ui *ui, Rectangle r)
{
    const struct wye_activity *a = &ui->st.activity;
    uint64_t now = plat_now_ms();
    float x = r.x;
    for (int i = 0; i < a->key_count; ++i) {
        float age = (float)(now - a->key_at[i]);
        if (age > KEY_LIFE_MS) {
            continue;
        }
        float t = age / KEY_LIFE_MS, pop = fminf(1, age / 120.0f);
        const char *name = ui_key_name(a->keys[i]);
        float w = fmaxf(48, ui_measure(name, 17, true).x + 24);
        if (x + w > r.x + r.width) {
            break;
        }
        Rectangle k = {x, r.y + 8 * (1 - pop), w, 44};
        ui_keycap(ui, k, name, t < 0.15f, ui_mix(UI_TARGET, UI_ACCENT, t));
        DrawRectangleRec((Rectangle){k.x - 2, k.y - 2, k.width + 4, k.height + 6}, Fade(UI_CARD, t * 0.9f));
        x += w + 8;
    }
}

void home_activity(struct wye_ui *ui, Rectangle card)
{
    Rectangle c = ui_card(ui, card, "Live");
    sample(ui);
    bool fwd = ui->st.mode != WYE_FWD_OFF;
    float pad = fminf(c.height - 70, 88), graph_h = 40;
    Rectangle left = {c.x, c.y, c.width - pad - 20, c.height - graph_h - 26};
    if (fwd) {
        keys(ui, (Rectangle){left.x, left.y + 4, left.width, 56});
    } else {
        ui_text(ui, "Keys and mouse moves show up here", left.x, left.y + 6, UI_FONT_SMALL, UI_MUTED);
        ui_text(ui, "while you control the other computer.", left.x, left.y + 26, UI_FONT_SMALL, UI_MUTED);
    }
    activity_pad((Rectangle){c.x + c.width - pad, c.y - 30, pad, pad}, g_vx, g_vy);
    activity_sound(ui, (Rectangle){left.x, left.y + left.height - 52, fminf(left.width, 340), 50});
    Rectangle g = {c.x, c.y + c.height - graph_h, c.width, graph_h};
    activity_graph(g, g_rate, HISTORY, g_head);
    ui_text(ui, TextFormat("%.0f messages/s", g_rate[(g_head + HISTORY - 1) % HISTORY]), c.x + c.width - 120,
            g.y - 20, UI_FONT_SMALL, UI_MUTED);
}
