/* Shortcuts: the switch hotkey, and hotkey + key actions that work on either computer. */
#include "ui.h"
#include "ui_theme.h"

#include "input/input.h"
#include "plat_input.h"

#include <stdio.h>

static const struct {
    enum wye_input_action action;
    const char *name;
} ROWS[] = {
    {WYE_ACT_JIGGLER, "Jiggler on/off"},          {WYE_ACT_PASTE, "Type the clipboard"},
    {WYE_ACT_CTRL_ALT_DEL, "Send Ctrl+Alt+Del"},   {WYE_ACT_RELEASE, "Release everything (panic)"},
    {WYE_ACT_MINI, "Mini panel / full window"},    {WYE_ACT_FOLDER, "Open the shared folder"},
    {WYE_ACT_SHOW_WINDOW, "Show Wye"},
};
#define ROW_COUNT ((int)(sizeof(ROWS) / sizeof(ROWS[0])))
#define LEARN_NONE (-1)
#define LEARN_HOTKEY (-2)

static int g_learning = LEARN_NONE; /* index into ROWS, or LEARN_HOTKEY */

void shortcuts_stop_learning(void)
{
    if (g_learning != LEARN_NONE) {
        g_learning = LEARN_NONE;
        wye_engine_learn_key(false, false);
    }
}

static const char *action_name(int action)
{
    for (int i = 0; i < ROW_COUNT; ++i) {
        if ((int)ROWS[i].action == action) {
            return ROWS[i].name;
        }
    }
    return "another action";
}

/* A key does one thing: whatever had it before gives it up, and the user hears about it. */
static void learned(struct wye_ui *ui)
{
    if (g_learning == LEARN_NONE || ui->st.learning_key || !ui->st.learned_key) {
        return;
    }
    uint8_t key = (uint8_t)ui->st.learned_key;
    int action = g_learning == LEARN_HOTKEY ? -1 : (int)ROWS[g_learning].action;
    int lost = wye_shortcut_assign(ui->cfg.shortcuts, WYE_ACT_COUNT, action, key);
    if (g_learning == LEARN_HOTKEY) {
        ui->cfg.hotkey = key;
    }
    g_learning = LEARN_NONE;
    ui_save_config(ui);
    if (lost) {
        ui_toast(ui, TextFormat("%s was also on that key: it is now off", action_name(lost)));
    }
}

static void row(struct wye_ui *ui, Rectangle c, float y, int index, const char *name, const char *keys)
{
    ui_text(ui, name, c.x, y + 9, UI_FONT, UI_TEXT);
    bool learning = g_learning == index;
    if (ui_button((Rectangle){c.x + c.width - 300, y, 190, UI_ROW}, learning ? "Press a key..." : keys)) {
        g_learning = learning ? LEARN_NONE : index;
        wye_engine_learn_key(!learning, index != LEARN_HOTKEY);
    }
    if (index >= 0 && ui_button((Rectangle){c.x + c.width - 100, y, 100, UI_ROW}, "Off")) {
        ui->cfg.shortcuts[ROWS[index].action] = 0;
        ui_save_config(ui);
    }
}

void page_shortcuts(struct wye_ui *ui, Rectangle a)
{
    learned(ui);
    char hot[32]; /* a copy: ui_key_name can reuse one buffer, and the rows name other keys */
    snprintf(hot, sizeof(hot), "%s", ui_key_name(ui->cfg.hotkey));
    ui_text_bold(ui, "Shortcuts", a.x, a.y - 4, 30, UI_TEXT);
    ui_text(ui, "They work whichever computer you are controlling, and neither computer sees these keys.",
            a.x + 158, a.y + 8, UI_FONT_SMALL, UI_MUTED);
    Rectangle card = {a.x, a.y + 56, a.width, a.height - 56};
    Rectangle c = ui_card(ui, card, NULL);
    ui_text(ui, TextFormat("Tap %s to switch computers. Hold it and press another key for an action.", hot), c.x, c.y,
            UI_FONT_SMALL, UI_MUTED);
    float y = c.y + 28;
    if (ui->st.capture != PLAT_INPUT_RUNNING) {
        ui_text(ui, "Wye cannot read your keyboard yet, so it cannot learn keys. See the Home page.", c.x, y,
                UI_FONT_SMALL, UI_WARN);
    } else if (g_learning != LEARN_NONE) {
        ui_text(ui, g_learning == LEARN_HOTKEY ? "Press the new hotkey now, or click the button again to cancel."
                                               : TextFormat("Press the key to use with %s now, or click the button "
                                                            "again to cancel.", hot),
                c.x, y, UI_FONT_SMALL, UI_ACCENT);
    }
    y += 28;
    row(ui, c, y, LEARN_HOTKEY, "Switch computers", hot);
    for (int i = 0; i < ROW_COUNT; ++i) {
        unsigned char key = ui->cfg.shortcuts[ROWS[i].action];
        y += UI_ROW + 6;
        if (y + UI_ROW > c.y + c.height) {
            break;
        }
        row(ui, c, y, i, ROWS[i].name, key ? TextFormat("%s + %s", hot, ui_key_name(key)) : "not set");
    }
}
