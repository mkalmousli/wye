/*
 * Home: the link map. This computer -> bridge Pico -> target Pico -> other
 * computer, each a node coloured by its state; links carry moving light
 * whose pace follows the messages really being sent.
 */
#include "home_map.h"
#include "ui_fx.h"
#include "ui_theme.h"
#include <math.h>

static float g_rate;        /* messages per second, smoothed */
static float g_phase[3];

static void measure_rate(const struct wye_ui *ui)
{
    static uint32_t last;
    float dt = GetFrameTime() > 0 ? GetFrameTime() : 0.016f;
    uint32_t now = ui->st.activity.messages;
    float inst = (float)(now - last) / dt;
    last = now;
    g_rate += (inst - g_rate) * (1.0f - expf(-dt * 4));
}

/* A link between two nodes; ok links flow, broken ones are dashed. */
static void link(int i, Vector2 a, Vector2 b, enum map_state s, const char *label, struct wye_ui *ui)
{
    Color col = s == MAP_OK ? UI_ACCENT : s == MAP_BAD ? UI_BAD : UI_CARD_EDGE;
    float len = b.x - a.x;
    if (s == MAP_OK) {
        ui_gradient_bar((Rectangle){a.x, a.y - 1.5f, len, 3}, UI_ACCENT, UI_ACCENT2);
        float speed = 0.25f + fminf(g_rate, 400) / 160.0f; /* idle links still breathe */
        int dots = 2 + (int)fminf(g_rate / 25.0f, 6);
        g_phase[i] = fmodf(g_phase[i] + speed * GetFrameTime(), 1.0f);
        for (int k = 0; k < dots; ++k) {
            float t = fmodf(g_phase[i] + (float)k / (float)dots, 1.0f);
            Vector2 p = {a.x + len * t, a.y};
            ui_glow(p, 9, UI_ACCENT2, 0.55f);
            DrawCircleV(p, 2.6f, WHITE);
        }
    } else {
        for (float x = a.x; x < b.x; x += 12) {
            DrawLineEx((Vector2){x, a.y}, (Vector2){fminf(x + 6, b.x), a.y}, 2, col);
        }
    }
    if (label && label[0]) {
        Vector2 m = ui_measure(label, 12, false);
        ui_text(ui, label, a.x + (len - m.x) / 2, a.y - 22, 12, s == MAP_OK ? UI_MUTED : col);
    }
}

void home_map(struct wye_ui *ui, Rectangle r)
{
    const struct wye_link_info *l = &ui->st.link;
    bool bridge = l->bridge.present, target = l->target.present;
    bool mounted = target && (l->target_state & WYE_TSTATE_USB_MOUNTED);
    bool asleep = target && (l->target_state & WYE_TSTATE_USB_SUSPENDED);
    bool input = ui->st.capture == PLAT_INPUT_RUNNING;
    measure_rate(ui);
    ui_card(ui, r, NULL);
    float cy = r.y + (r.height - 124) / 2 + 30, step = (r.width - 120) / 3; /* nodes + labels + LEDs: 124 px */
    struct map_node n[4] = {
        {MAP_COMPUTER, "This computer", input ? "keyboard & mouse ready" : "input blocked", input ? MAP_OK : MAP_WARN, BLANK},
        {MAP_PICO, ui_pico_name(ui, WYE_NODE_BRIDGE, "Bridge Pico"), bridge ? map_version(&l->bridge) : "not found",
         bridge ? MAP_OK : MAP_BAD, ui_pico_color(ui, WYE_NODE_BRIDGE, UI_TEXT)},
        {MAP_PICO, ui_pico_name(ui, WYE_NODE_TARGET, "Target Pico"), target ? map_version(&l->target) : bridge ? "not answering" : "-",
         target ? MAP_OK : bridge ? MAP_BAD : MAP_OFF, ui_pico_color(ui, WYE_NODE_TARGET, UI_TEXT)},
        {MAP_COMPUTER, "Other computer", asleep ? "asleep" : mounted ? "connected" : "-",
         asleep ? MAP_WARN : mounted ? MAP_OK : MAP_OFF, BLANK},
    };
    Vector2 at[4];
    for (int i = 0; i < 4; ++i) {
        at[i] = (Vector2){r.x + 60 + step * (float)i, cy};
    }
    const char *labels[3] = {"USB", target ? TextFormat("wires  %u ms", l->target.rtt_ms) : "wires", "USB"};
    enum map_state ls[3] = {bridge ? MAP_OK : MAP_BAD, target ? MAP_OK : bridge ? MAP_BAD : MAP_OFF,
                            mounted ? MAP_OK : MAP_OFF};
    for (int i = 0; i < 3; ++i) {
        link(i, (Vector2){at[i].x + 34, cy}, (Vector2){at[i + 1].x - 34, cy}, ls[i], labels[i], ui);
    }
    for (int i = 0; i < 4; ++i) {
        map_node_draw(ui, &n[i], at[i], ui->st.mode != WYE_FWD_OFF && i == 3);
    }
    if (target) {
        map_leds(ui, (Vector2){at[3].x, cy + 66}, l->leds);
    }
}
