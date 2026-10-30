/* Setup: the install screen - stepper, checklist, animated scene, words. */
#include "setup_flash.h"
#include "ui_theme.h"
#include <math.h>

#define LOOP_S 4.4f

void flash_words(struct wye_ui *ui, const struct flash_state *s, Rectangle r, enum flash_action *action);

static float ease(float t)
{
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    return t * t * (3 - 2 * t);
}

/* Which of the three stages is active (3 = this Pico is finished). */
static int stage(const struct flash_state *s)
{
    switch (s->phase) {
    case FLASH_WAITING: case FLASH_OPENING: case FLASH_STUCK: case FLASH_RESCUE: return 0;
    case FLASH_ASK: return 1;
    case FLASH_DONE: case FLASH_ALL_SET: return 3;
    default: return 2;
    }
}

static void stepper(struct wye_ui *ui, Rectangle r, int active)
{
    static const char *const NAMES[] = {"Plug in with BOOTSEL", "Which Pico?", "Install"};
    float w = r.width / 3;
    for (int i = 0; i < 3; ++i) {
        Vector2 c = {r.x + w * (float)i + 16, r.y + 16};
        float label_end = c.x + 24 + ui_measure(NAMES[i], UI_FONT, true).x + 12;
        if (i < 2 && label_end < c.x + w - 24) {
            DrawLineEx((Vector2){label_end, c.y}, (Vector2){c.x + w - 24, c.y}, 2, i < active ? UI_OK : UI_CARD_EDGE);
        }
        if (i < active) {
            art_check(c, 14, UI_OK);
        } else {
            Color col = i == active ? UI_ACCENT : UI_CARD_EDGE;
            float pulse = i == active ? 4.0f * (0.5f + 0.5f * sinf((float)GetTime() * 5)) : 0;
            DrawCircleV(c, 14 + pulse, Fade(col, 0.25f));
            DrawCircleV(c, 14, col);
            ui_text_bold(ui, TextFormat("%d", i + 1), c.x - 4, c.y - 10, UI_FONT, RAYWHITE);
        }
        ui_text_bold(ui, NAMES[i], c.x + 24, c.y - 10, UI_FONT, i <= active ? UI_TEXT : UI_MUTED);
    }
}

/* "Bridge / Target" pills: what is finished across both Picos. */
static void checklist(struct wye_ui *ui, const struct flash_state *s, Vector2 at)
{
    for (uint8_t role = WYE_ROLE_BRIDGE; role <= WYE_ROLE_TARGET; ++role) {
        Rectangle p = {at.x, at.y + (float)(role - 1) * 34, 128, 28};
        Color col = role == WYE_ROLE_TARGET ? UI_TARGET : UI_ACCENT;
        DrawRectangleRounded(p, 1.0f, 8, Fade(col, s->done[role] ? 0.3f : 0.1f));
        if (s->done[role]) {
            art_check((Vector2){p.x + 14, p.y + 14}, 9, UI_OK);
        } else {
            DrawRing((Vector2){p.x + 14, p.y + 14}, 7, 9, 0, 360, 24, UI_MUTED);
        }
        ui_text_bold(ui, role == WYE_ROLE_TARGET ? "Target" : "Bridge", p.x + 30, p.y + 5, UI_FONT_SMALL, UI_TEXT);
    }
}

static struct art_scene scene_for(const struct flash_state *s, float age)
{
    struct art_scene sc = {.role = s->role, .plug = 1.0f, .led = BLANK};
    float u = fmodf(age, LOOP_S);
    switch (s->phase) {
    case FLASH_WAITING:
        /* Loop: finger presses and holds BOOTSEL, cable goes in, finger lets go. */
        sc.press = u < 3.6f ? ease(u / 0.6f) : 1.0f - ease((u - 3.6f) / 0.5f);
        sc.plug = u < 1.3f ? 0.0f : (u < 3.9f ? ease((u - 1.3f) / 0.9f) : 0.0f);
        break;
    case FLASH_INSTALLING: case FLASH_UPDATING: sc.led = fmod(GetTime(), 0.3) < 0.15 ? UI_OK : BLANK; break;
    case FLASH_DONE: case FLASH_ALL_SET: sc.led = UI_OK; sc.check = ease(age / 0.45f); break;
    case FLASH_FAILED: sc.led = UI_BAD; break;
    default: break;
    }
    return sc;
}

enum flash_action flash_view(struct wye_ui *ui, const struct flash_state *s, Rectangle c)
{
    float age = (float)(GetTime() - s->since), words_h = 108;
    stepper(ui, (Rectangle){c.x, c.y, c.width, 36}, stage(s));
    Rectangle scene = {c.x, c.y + 52, c.width - 140, c.height - 52 - words_h};
    struct art_scene sc = scene_for(s, age);
    art_scene(ui, scene, &sc);
    checklist(ui, s, (Vector2){c.x + c.width - 128, scene.y + 8});
    if (s->phase == FLASH_OPENING || s->phase == FLASH_RESCUE) {
        art_spinner((Vector2){scene.x + scene.width * 0.75f, scene.y + scene.height / 2}, 26, UI_ACCENT);
    }
    enum flash_action action = FLASH_ACT_NONE;
    flash_words(ui, s, (Rectangle){c.x, c.y + c.height - words_h, c.width, words_h}, &action);
    ui_confetti_draw();
    return action;
}
