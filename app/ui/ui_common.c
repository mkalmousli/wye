/* Toasts, settings persistence and key names. */
#include "ui.h"
#include "ui_theme.h"

#include "util/util.h"

#include <stdio.h>

void ui_toast(struct wye_ui *ui, const char *text)
{
    if (ui->toast_count == UI_TOASTS) {
        for (int i = 1; i < UI_TOASTS; ++i) {
            ui->toasts[i - 1] = ui->toasts[i];
        }
        ui->toast_count--;
    }
    wye_strcpy(ui->toasts[ui->toast_count].text, WYE_NOTICE_LEN, text);
    ui->toasts[ui->toast_count].until = GetTime() + 5.0;
    ui->toast_count++;
}

void ui_draw_toasts(struct wye_ui *ui)
{
    float y = UI_PAD + 40;
    for (int i = ui->toast_count - 1; i >= 0; --i) {
        if (GetTime() > ui->toasts[i].until) {
            for (int k = i + 1; k < ui->toast_count; ++k) {
                ui->toasts[k - 1] = ui->toasts[k];
            }
            ui->toast_count--;
            continue;
        }
        Vector2 size = ui_measure(ui->toasts[i].text, UI_FONT_SMALL, false);
        Rectangle r = {(float)GetScreenWidth() - size.x - 3 * UI_PAD, y - 40, size.x + 2 * UI_PAD, 36};
        DrawRectangleRounded(r, 0.3f, 8, (Color){44, 52, 70, 240});
        DrawRectangleRoundedLinesEx(r, 0.3f, 8, 1, UI_ACCENT);
        ui_text(ui, ui->toasts[i].text, r.x + UI_PAD, r.y + 10, UI_FONT_SMALL, UI_TEXT);
        y += 46;
    }
}

void ui_save_config(struct wye_ui *ui)
{
    wye_config_sanitize(&ui->cfg);
    wye_config_save(&ui->cfg);
    wye_engine_apply_config(&ui->cfg);
    wye_updater_configure(&ui->cfg);
}

static const char *const NAMED[][2] = {
    {"\x28", "Enter"}, {"\x29", "Esc"}, {"\x2a", "Backspace"}, {"\x2b", "Tab"}, {"\x2c", "Space"},
    {"\x39", "Caps Lock"}, {"\x46", "Print Screen"}, {"\x47", "Scroll Lock"}, {"\x48", "Pause"},
    {"\x49", "Insert"}, {"\x4a", "Home"}, {"\x4b", "Page Up"}, {"\x4c", "Delete"}, {"\x4d", "End"},
    {"\x4e", "Page Down"}, {"\x53", "Num Lock"}, {"\x65", "Menu"}, {"\xe0", "Left Ctrl"},
    {"\xe1", "Left Shift"}, {"\xe2", "Left Alt"}, {"\xe3", "Left Super"}, {"\xe4", "Right Ctrl"},
    {"\xe5", "Right Shift"}, {"\xe6", "Right Alt"}, {"\xe7", "Right Super"},
};

const char *ui_key_name(int usage)
{
    static char buf[16];
    if (usage >= 0x04 && usage <= 0x1d) {
        snprintf(buf, sizeof(buf), "%c", 'A' + usage - 0x04);
    } else if (usage >= 0x1e && usage <= 0x27) {
        snprintf(buf, sizeof(buf), "%d", (usage - 0x1d) % 10);
    } else if (usage >= 0x3a && usage <= 0x45) {
        snprintf(buf, sizeof(buf), "F%d", usage - 0x39);
    } else if (usage >= 0x68 && usage <= 0x73) {
        snprintf(buf, sizeof(buf), "F%d", usage - 0x68 + 13);
    } else {
        for (size_t i = 0; i < WYE_ARRAY_LEN(NAMED); ++i) {
            if ((unsigned char)NAMED[i][0][0] == usage) {
                return NAMED[i][1];
            }
        }
        snprintf(buf, sizeof(buf), "Key 0x%02x", usage);
    }
    return buf;
}

void ui_open_shared_folder(struct wye_ui *ui)
{
    if (ui->st.drive.enabled) {
        plat_reveal_path(ui->st.drive.folder);
    } else {
        ui_toast(ui, "The shared folder is off. Turn it on under Devices.");
    }
}
