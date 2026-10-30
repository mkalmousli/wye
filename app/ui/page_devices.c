/* Devices: what the other computer gets, as tabs - overview switches, input, drive, sound, identity, shortcuts. */
#include "ui.h"
#include "ui_theme.h"

#include "devices.h"

static const char *const TABS[] = {"Overview", "Keyboard & mouse", "Shared drive", "Sound", "USB identity", "Shortcuts"};

void page_devices(struct wye_ui *ui, Rectangle a)
{
    ui_text_bold(ui, "Devices", a.x, a.y - 4, 30, UI_TEXT);
    ui_text(ui, "What the other computer sees, and how it behaves.", a.x + 132, a.y + 8, UI_FONT_SMALL, UI_MUTED);
    devices_functions_sync(ui);
    Rectangle tabs = {a.x, a.y + 48, a.width < 860 ? a.width : 860, 38};
    ui->devices_tab = ui_tabs(ui, tabs, TABS, 6, ui->devices_tab);
    Rectangle body = {a.x, a.y + 102, a.width, a.height - 102};
    switch (ui->devices_tab) {
    case 0: devices_overview(ui, body); break;
    case 1: devices_input(ui, body); break;
    case 2: devices_folder(ui, ui_card(ui, body, NULL)); break;
    case 3: devices_audio(ui, ui_card(ui, body, NULL)); break;
    case 4: identity_panel(ui, body); break;
    default: devices_shortcuts(ui, ui_card(ui, body, NULL)); break;
    }
}
