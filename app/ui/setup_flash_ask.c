/* Setup: "Which Pico is this?" - asked before anything is written. */
#include "setup_flash.h"
#include "ui_theme.h"

/* FLASH_ASK: two big answers; the Pico that is still missing is suggested. */
void flash_ask(struct wye_ui *ui, const struct flash_state *s, Rectangle r, enum flash_action *action)
{
    ui_text_bold(ui, "Which Pico is this?", r.x, r.y, 24, UI_TEXT);
    ui_text(ui, "The bridge stays at this computer. The target goes to the other computer.", r.x, r.y + 34,
            UI_FONT, UI_MUTED);
    float w = (r.width - 12) / 2 > 300 ? 300 : (r.width - 12) / 2, y = r.y + 62;
    bool suggest_bridge = !s->done[WYE_ROLE_BRIDGE], suggest_target = s->done[WYE_ROLE_BRIDGE];
    const char *b = s->done[WYE_ROLE_BRIDGE] ? "Bridge (again)" : "Bridge - stays here";
    const char *t = s->done[WYE_ROLE_TARGET] ? "Target (again)" : "Target - goes to the other PC";
    if (ui_button_tinted((Rectangle){r.x, y, w, 40}, b, suggest_bridge ? UI_ACCENT : UI_CARD_EDGE)) {
        *action = FLASH_ACT_PICK_BRIDGE;
    }
    if (ui_button_tinted((Rectangle){r.x + w + 12, y, w, 40}, t, suggest_target ? UI_TARGET : UI_CARD_EDGE)) {
        *action = FLASH_ACT_PICK_TARGET;
    }
}
