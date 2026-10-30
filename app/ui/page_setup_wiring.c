/* Setup: wiring diagram and the final connection test. */
#include "ui.h"
#include "ui_theme.h"

static void pico(struct wye_ui *ui, Rectangle r, const char *title)
{
    DrawRectangleRounded(r, 0.12f, 8, (Color){20, 92, 60, 255});
    ui_text_bold(ui, title, r.x + 12, r.y + 10, UI_FONT, UI_TEXT);
    const char *pins[] = {"GP0 (TX)", "GP1 (RX)", "GND"};
    for (int i = 0; i < 3; ++i) {
        float y = r.y + 50 + (float)i * 34;
        bool left = r.x > 300;
        float px = left ? r.x : r.x + r.width;
        DrawCircleV((Vector2){px, y + 9}, 7, GOLD);
        ui_text(ui, pins[i], left ? r.x + 16 : r.x + r.width - 96, y, UI_FONT_SMALL, UI_TEXT);
    }
}

void setup_wiring_step(struct wye_ui *ui, Rectangle c)
{
    Rectangle a = {c.x + 20, c.y + 10, 220, 170}, b = {c.x + c.width - 240, c.y + 10, 220, 170};
    pico(ui, a, "Bridge Pico");
    pico(ui, b, "Target Pico");
    Color wires[] = {UI_ACCENT, UI_WARN, UI_MUTED};
    int to[] = {1, 0, 2}; /* TX->RX, RX->TX, GND->GND */
    for (int i = 0; i < 3; ++i) {
        Vector2 from = {a.x + a.width, a.y + 59 + (float)i * 34};
        Vector2 dest = {b.x, b.y + 59 + (float)to[i] * 34};
        DrawLineBezier(from, dest, 3, wires[i]);
    }
    float y = c.y + 200;
    ui_text(ui, "Connect three wires: GP0 to GP1, GP1 to GP0, and GND to GND (crossed TX/RX).",
            c.x, y, UI_FONT, UI_TEXT);
    ui_text(ui, "Plug the bridge Pico into this computer and the target Pico into the other computer.",
            c.x, y + 30, UI_FONT, UI_TEXT);
}

bool setup_test_step(struct wye_ui *ui, Rectangle c)
{
    const struct wye_link_info *l = &ui->st.link;
    const char *steps[] = {"Bridge found", "Target answering", "Target computer accepts input"};
    bool ok[] = {l->bridge.present, l->target.present, (l->target_state & WYE_TSTATE_USB_MOUNTED) != 0};
    for (int i = 0; i < 3; ++i) {
        float y = c.y + (float)i * 34;
        ui_dot(c.x + 8, y + 10, ok[i] ? UI_OK : UI_CARD_EDGE);
        ui_text(ui, steps[i], c.x + 26, y, UI_FONT, ok[i] ? UI_TEXT : UI_MUTED);
    }
    bool all = ok[0] && ok[1] && ok[2];
    const char *msg = all ? TextFormat("All set! Press %s to control the other computer, and again to come back.",
                                       ui_key_name(ui->cfg.hotkey))
                    : !ok[0] ? "Plug the bridge Pico into this computer."
                    : !ok[1] ? "Check the three wires and that the target Pico is powered."
                             : "Plug the target Pico into the other computer.";
    ui_text(ui, msg, c.x, c.y + 120, UI_FONT, all ? UI_OK : UI_WARN);
    return all;
}
