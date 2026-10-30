/* Devices: shortcuts - the switch hotkey and hotkey + key actions, which work on either computer. */
#include "devices.h"
#include "ui_theme.h"

#include "raygui.h"

static const struct {
    enum wye_input_action action;
    const char *name;
} ROWS[] = {
    {WYE_ACT_JIGGLER, "Jiggler on/off"},          {WYE_ACT_PASTE, "Type the clipboard"},
    {WYE_ACT_CTRL_ALT_DEL, "Send Ctrl+Alt+Del"},   {WYE_ACT_RELEASE, "Release everything (panic)"},
    {WYE_ACT_MINI, "Mini panel / full window"},    {WYE_ACT_FOLDER, "Open the shared folder"},
    {WYE_ACT_SHOW_WINDOW, "Show Wye"},
};

static int g_learning = -1; /* index into ROWS, or -2 for the switch hotkey */

static void learned(struct wye_ui *ui)
{
    if (g_learning == -1 || ui->st.learning_key || !ui->st.learned_key) {
        return;
    }
    if (g_learning == -2) {
        ui->cfg.hotkey = ui->st.learned_key;
    } else {
        ui->cfg.shortcuts[ROWS[g_learning].action] = (unsigned char)ui->st.learned_key;
    }
    g_learning = -1;
    ui_save_config(ui);
    wye_engine_apply_config(&ui->cfg);
}

static void row(struct wye_ui *ui, Rectangle c, float y, int index, const char *name, const char *keys)
{
    ui_text(ui, name, c.x, y + 9, UI_FONT, UI_TEXT);
    bool learning = g_learning == index;
    if (ui_button((Rectangle){c.x + c.width - 300, y, 190, UI_ROW}, learning ? "Press a key..." : keys)) {
        g_learning = learning ? -1 : index;
        wye_engine_learn_key(!learning);
    }
    if (index >= 0 && ui_button((Rectangle){c.x + c.width - 100, y, 100, UI_ROW}, "Off")) {
        ui->cfg.shortcuts[ROWS[index].action] = 0;
        ui_save_config(ui);
        wye_engine_apply_config(&ui->cfg);
    }
}

void devices_shortcuts(struct wye_ui *ui, Rectangle c)
{
    const char *hot = ui_key_name(ui->cfg.hotkey);
    learned(ui);
    ui_text(ui, TextFormat("Tap %s to switch computers. Hold %s and press a key for an action - this works", hot, hot),
            c.x, c.y, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, "whether you are controlling the other computer or not. Neither computer sees these keys.", c.x,
            c.y + 20, UI_FONT_SMALL, UI_MUTED);
    float y = c.y + 52;
    row(ui, c, y, -2, "Switch computers", hot);
    for (int i = 0; i < (int)(sizeof(ROWS) / sizeof(ROWS[0])) && y + 2 * UI_ROW < c.y + c.height; ++i) {
        unsigned char key = ui->cfg.shortcuts[ROWS[i].action];
        y += UI_ROW + 6;
        row(ui, c, y, i, ROWS[i].name, key ? TextFormat("%s + %s", hot, ui_key_name(key)) : "not set");
    }
}
