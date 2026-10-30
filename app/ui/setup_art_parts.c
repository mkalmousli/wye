/* Setup artwork: a Raspberry Pi Pico, a laptop, a finger, ticks and spinners. */
#include "setup_flash.h"
#include "ui_theme.h"
#include <math.h>

#define PCB (Color){24, 122, 74, 255}
#define PCB_EDGE (Color){14, 84, 50, 255}
#define PIN_GOLD (Color){214, 176, 82, 255}
#define SILVER (Color){196, 202, 214, 255}

Vector2 art_bootsel_pos(Rectangle b) { return (Vector2){b.x + b.width * 0.235f, b.y + b.height * 0.66f}; }
Vector2 art_usb_pos(Rectangle b) { return (Vector2){b.x - b.width * 0.035f, b.y + b.height * 0.5f}; }

static void pins(Rectangle b)
{
    for (int i = 0; i < 20; ++i) {
        float x = b.x + b.width * (0.075f + (float)i * 0.0445f), w = b.width * 0.022f, h = b.height * 0.1f;
        DrawRectangleRec((Rectangle){x, b.y + b.height * 0.03f, w, h}, PIN_GOLD);
        DrawRectangleRec((Rectangle){x, b.y + b.height * 0.87f, w, h}, PIN_GOLD);
    }
}

void art_pico(struct wye_ui *ui, Rectangle b, uint8_t role, Color led)
{
    Vector2 usb = art_usb_pos(b), bs = art_bootsel_pos(b);
    DrawRectangleRec((Rectangle){usb.x, usb.y - b.height * 0.15f, b.width * 0.1f, b.height * 0.3f}, SILVER);
    DrawRectangleRounded(b, 0.1f, 8, PCB);
    DrawRectangleRoundedLinesEx(b, 0.1f, 8, 2, PCB_EDGE);
    pins(b);
    Rectangle chip = {b.x + b.width * 0.46f, b.y + b.height * 0.29f, b.height * 0.42f, b.height * 0.42f};
    DrawRectangleRounded(chip, 0.12f, 4, (Color){30, 32, 36, 255});
    ui_text(ui, "RP2040", chip.x + chip.width * 0.12f, chip.y + chip.height * 0.4f, chip.height * 0.2f, UI_MUTED);
    Rectangle led_r = {b.x + b.width * 0.13f, b.y + b.height * 0.24f, b.width * 0.03f, b.height * 0.09f};
    if (led.a) {
        DrawCircleV((Vector2){led_r.x + led_r.width / 2, led_r.y + led_r.height / 2}, b.height * 0.13f, Fade(led, 0.35f));
    }
    DrawRectangleRec(led_r, led.a ? led : (Color){210, 220, 210, 255});
    float s = b.height * 0.2f;
    DrawRectangleRounded((Rectangle){bs.x - s / 2, bs.y - s / 2, s, s}, 0.25f, 6, RAYWHITE);
    DrawCircleV(bs, s * 0.28f, (Color){200, 200, 200, 255});
    float label = fmaxf(10.0f, b.height * 0.1f);
    ui_text_bold(ui, "BOOTSEL", bs.x - label * 2.0f, bs.y + s * 0.6f, label, RAYWHITE);
    Color tag = role == WYE_ROLE_TARGET ? UI_TARGET : role == WYE_ROLE_BRIDGE ? UI_ACCENT : UI_CARD_EDGE;
    Rectangle t = {b.x + b.width * 0.72f, b.y + b.height * 0.34f, b.width * 0.22f, b.height * 0.32f};
    DrawRectangleRounded(t, 0.3f, 6, tag);
    const char *name = role == WYE_ROLE_TARGET ? "TARGET" : role == WYE_ROLE_BRIDGE ? "BRIDGE" : "?";
    float fs = t.height * 0.5f;
    Vector2 m = ui_measure(name, fs, true);
    ui_text_bold(ui, name, t.x + (t.width - m.x) / 2, t.y + (t.height - m.y) / 2, fs, RAYWHITE);
}

/* A laptop; returns where its USB port is. */
Vector2 art_computer(Rectangle r)
{
    Rectangle screen = {r.x, r.y, r.width, r.height * 0.74f};
    DrawRectangleRounded(screen, 0.06f, 6, (Color){52, 60, 76, 255});
    Rectangle glass = {screen.x + 6, screen.y + 6, screen.width - 12, screen.height - 12};
    DrawRectangleRec(glass, (Color){20, 28, 44, 255});
    for (int i = 0; i < 3; ++i) { /* a hint of a desktop */
        DrawRectangleRounded((Rectangle){glass.x + 10, glass.y + 10 + (float)i * glass.height * 0.22f,
                                         glass.width * (0.7f - (float)i * 0.18f), glass.height * 0.1f},
                             1.0f, 6, Fade(UI_ACCENT, 0.35f));
    }
    float y0 = r.y + r.height * 0.78f, y1 = r.y + r.height;
    Color base = (Color){84, 94, 112, 255};
    DrawTriangle((Vector2){r.x, y0}, (Vector2){r.x - r.width * 0.08f, y1}, (Vector2){r.x + r.width * 1.08f, y1}, base);
    DrawTriangle((Vector2){r.x, y0}, (Vector2){r.x + r.width * 1.08f, y1}, (Vector2){r.x + r.width, y0}, base);
    Vector2 port = {r.x + r.width * 1.04f, (y0 + y1) / 2};
    DrawRectangleRec((Rectangle){port.x - 4, port.y - 3, 8, 6}, (Color){20, 22, 28, 255});
    return port;
}

void art_finger(struct wye_ui *ui, Vector2 button, float press)
{
    float lift = (1.0f - press) * 46.0f, w = 26.0f;
    Color skin = Fade((Color){242, 200, 168, 255}, fminf(1.0f, press * 3.0f));
    Rectangle f = {button.x - w / 2, button.y - 8 - lift - 70, w, 70};
    DrawRectangleRounded(f, 0.5f, 10, skin);
    DrawRectangleRounded((Rectangle){f.x + 6, f.y + f.height - 22, w - 12, 14}, 0.5f, 6, Fade(RAYWHITE, skin.a / 400.0f));
    if (press > 0.95f) {
        float pulse = (float)fmod(GetTime(), 0.8) / 0.8f;
        DrawRing(button, 14 + pulse * 22, 16 + pulse * 22, 0, 360, 40, Fade(UI_WARN, 1.0f - pulse));
        ui_text_bold(ui, "HOLD", f.x + w + 8, f.y + f.height - 24, UI_FONT, UI_WARN);
    }
}

void art_check(Vector2 c, float radius, Color col)
{
    DrawCircleV(c, radius * 1.25f, Fade(col, 0.2f));
    DrawCircleV(c, radius, col);
    float s = radius * 0.5f, t = fmaxf(3.0f, radius * 0.16f);
    DrawLineEx((Vector2){c.x - s, c.y}, (Vector2){c.x - s * 0.25f, c.y + s * 0.7f}, t, RAYWHITE);
    DrawLineEx((Vector2){c.x - s * 0.25f, c.y + s * 0.7f}, (Vector2){c.x + s, c.y - s * 0.6f}, t, RAYWHITE);
}

void art_spinner(Vector2 c, float radius, Color col)
{
    float a = (float)fmod(GetTime() * 300.0, 360.0);
    DrawRing(c, radius - 4, radius, a, a + 270, 36, col);
}
