/* Guided setup wizard: permissions, flashing both Picos, wiring, test. */
#include "ui.h"
#include "ui_theme.h"

#include "plat_sys.h"
#include "raygui.h"

enum { STEP_WELCOME, STEP_PERMISSION, STEP_FIRMWARE, STEP_WIRING, STEP_TEST, STEP_COUNT };

bool setup_flash_step(struct wye_ui *ui, Rectangle c);
void setup_wiring_step(struct wye_ui *ui, Rectangle c);
bool setup_test_step(struct wye_ui *ui, Rectangle c);

static const char *const TITLES[STEP_COUNT] = {
    "Welcome", "Allow keyboard & mouse access", "Install the firmware on both Picos",
    "Connect the cables", "Test the connection",
};

static void lines(struct wye_ui *ui, Rectangle c, const char *const *text, int n)
{
    for (int i = 0; i < n; ++i) {
        ui_text(ui, text[i], c.x, c.y + (float)i * 28, UI_FONT, UI_TEXT);
    }
}

static bool welcome(struct wye_ui *ui, Rectangle c)
{
    static const char *const text[] = {
        "Wye lets this computer's keyboard and mouse control another computer",
        "through a cable - no network, no software on the other computer.",
        "",
        "You need: two Raspberry Pi Pico boards, three jumper wires and two USB cables.",
        "This wizard installs the firmware on both Picos and checks the connection.",
    };
    lines(ui, c, text, 5);
    return true;
}

static bool permission(struct wye_ui *ui, Rectangle c)
{
    if (plat_input_permission() != PLAT_PERM_NEEDED) {
        ui_text(ui, "Access is granted. Continue.", c.x, c.y, UI_FONT, UI_OK);
        return true;
    }
    ui_text(ui, "Your system must allow Wye to read and block keyboard/mouse input", c.x, c.y, UI_FONT, UI_TEXT);
    ui_text(ui, "while you control the other computer.", c.x, c.y + 28, UI_FONT, UI_TEXT);
    char err[160] = "";
    if (ui_button_primary((Rectangle){c.x, c.y + 72, 220, UI_ROW}, "Grant access") &&
        !plat_input_permission_request(err, sizeof(err))) {
        ui_toast(ui, err[0] ? err : "Access was not granted");
    }
    return false;
}

void page_setup(struct wye_ui *ui, Rectangle a)
{
    int step = ui->setup_step < 0 ? 0 : (ui->setup_step >= STEP_COUNT ? STEP_COUNT - 1 : ui->setup_step);
    ui_text_bold(ui, TextFormat("Setup - step %d of %d", step + 1, STEP_COUNT), a.x, a.y, UI_FONT_TITLE, UI_TEXT);
    Rectangle c = ui_card(ui, (Rectangle){a.x, a.y + 44, a.width, a.height - 110}, TITLES[step]);
    bool can_next = false;
    switch (step) {
    case STEP_WELCOME: can_next = welcome(ui, c); break;
    case STEP_PERMISSION: can_next = permission(ui, c); break;
    case STEP_FIRMWARE: can_next = setup_flash_step(ui, c); break;
    case STEP_WIRING: setup_wiring_step(ui, c); can_next = true; break;
    default: can_next = setup_test_step(ui, c); break;
    }
    float y = a.y + a.height - 44;
    if (ui_button((Rectangle){a.x, y, 120, UI_ROW}, step == 0 ? "Close" : "Back")) {
        ui->setup_step = step - 1;
        ui->page = step == 0 ? PAGE_HOME : PAGE_SETUP;
    }
    if (step == STEP_FIRMWARE) {
        if (ui_button((Rectangle){a.x + a.width - 300, y, 160, UI_ROW}, "Skip (already done)")) {
            ui->setup_step = step + 1;
        }
    }
    if (!can_next) {
        GuiDisable();
    }
    const char *next = step == STEP_COUNT - 1 ? "Finish" : "Next";
    if (ui_button_primary((Rectangle){a.x + a.width - 130, y, 130, UI_ROW}, next)) {
        ui->setup_step = step + 1;
        ui->page = step == STEP_COUNT - 1 ? PAGE_HOME : PAGE_SETUP;
    }
    GuiEnable();
}
