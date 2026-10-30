/* Setup artwork: the whole scene - laptop, USB cable and Pico. */
#include "setup_flash.h"
#include "ui_theme.h"
#include <math.h>

static void cable(Vector2 port, Vector2 socket, float plug)
{
    float plug_len = 30.0f;
    Vector2 tip = {socket.x - (1.0f - plug) * 90.0f, socket.y};
    Vector2 back = {tip.x - plug_len, tip.y};
    DrawLineBezier(port, back, 6, (Color){38, 42, 50, 255});
    DrawLineBezier(port, back, 3, (Color){96, 104, 120, 255});
    DrawRectangleRounded((Rectangle){back.x, back.y - 9, plug_len - 8, 18}, 0.3f, 6, (Color){60, 66, 78, 255});
    DrawRectangleRec((Rectangle){tip.x - 9, tip.y - 5, 9, 10}, (Color){196, 202, 214, 255});
    if (plug >= 1.0f) {
        DrawCircleV(socket, 5, UI_OK);
        DrawCircleV(socket, 11, Fade(UI_OK, 0.25f));
    }
}

void art_scene(struct wye_ui *ui, Rectangle r, const struct art_scene *sc)
{
    float pc_w = fminf(r.width * 0.24f, r.height * 1.1f), pc_h = pc_w * 0.72f;
    Rectangle pc = {r.x + r.width * 0.02f, r.y + (r.height - pc_h) / 2, pc_w, pc_h};
    float b_w = fminf(r.width * 0.42f, r.height * 1.9f), b_h = b_w / 2.45f;
    Rectangle board = {r.x + r.width * 0.54f, r.y + (r.height - b_h) / 2, b_w, b_h};
    Vector2 port = art_computer(pc);
    ui_text(ui, "This computer", pc.x, pc.y + pc.height + 10, UI_FONT_SMALL, UI_MUTED);
    art_pico(ui, board, sc->role, sc->led);
    cable(port, art_usb_pos(board), sc->plug);
    if (sc->press > 0.0f) {
        art_finger(ui, art_bootsel_pos(board), sc->press);
    }
    if (sc->check > 0.0f) {
        Vector2 c = {board.x + board.width / 2, board.y + board.height / 2};
        float k = sc->check, overshoot = 1.0f + 0.25f * sinf(k * 3.14159f); /* pops in, settles */
        art_check(c, board.height * 0.36f * k * overshoot, UI_OK);
    }
}
