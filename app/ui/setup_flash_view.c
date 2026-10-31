/* Setup: the install screen - checklist, animated scene, words. */
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
    struct art_scene sc = {.role = s->role, .plug = 1.0f, .led = BLANK, .check = 0.0f};
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
    Rectangle scene = {c.x, c.y, c.width - 140, c.height - words_h};
    struct art_scene sc = scene_for(s, age);
    art_scene(ui, scene, &sc);
    checklist(ui, s, (Vector2){c.x + c.width - 128, scene.y + 8});
    enum flash_action action = FLASH_ACT_NONE;
    /* The first Pico becomes the bridge. Only a Pico done over again needs this. */
    ui_text(ui, "Next Pico", c.x + c.width - 128, scene.y + 86, UI_FONT_SMALL, UI_MUTED);
    int next = ui_segmented((Rectangle){c.x + c.width - 128, scene.y + 108, 128, 28}, "Auto;B;T", s->next_role);
    if (next != s->next_role) {
        action = next == WYE_ROLE_BRIDGE ? FLASH_ACT_NEXT_BRIDGE : next == WYE_ROLE_TARGET ? FLASH_ACT_NEXT_TARGET
                                                                                           : FLASH_ACT_NEXT_AUTO;
    }
    if (s->phase == FLASH_OPENING || s->phase == FLASH_RESCUE) {
        art_spinner((Vector2){scene.x + scene.width * 0.75f, scene.y + scene.height / 2}, 26, UI_ACCENT);
    }
    flash_words(ui, s, (Rectangle){c.x, c.y + c.height - words_h, c.width, words_h}, &action);
    ui_confetti_draw();
    return action;
}
