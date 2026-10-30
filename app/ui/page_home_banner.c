/* Home: the banner with the most important problem (or running update) and its fix. */
#include "ui.h"
#include "ui_theme.h"

#include "plat_sys.h"
#include "raygui.h"

/* Shows the most important problem with a fix button; true if drawn. */
bool home_banner(struct wye_ui *ui, Rectangle r)
{
    const char *msg = NULL, *action = NULL;
    enum wye_page go = PAGE_HOME;
    if (ui->st.fw_phase >= WYE_FWS_BEGIN && ui->st.fw_phase <= WYE_FWS_REBOOT) {
        msg = TextFormat("Updating the %s firmware over USB (%d%%) - no button needed, keep it plugged in.",
                         ui->st.fw_node == WYE_NODE_TARGET ? "target" : "bridge", ui->st.fw_progress);
        action = "Details";
        go = PAGE_DEVICE;
    } else if (ui->st.capture != PLAT_INPUT_RUNNING && plat_input_permission() == PLAT_PERM_NEEDED) {
        msg = "Wye needs permission to read your keyboard and mouse.";
        action = "Grant access";
    } else if (ui->st.link.state == WYE_LINK_LEGACY || ui->st.bridge_outdated || ui->st.target_outdated) {
        msg = ui->cfg.auto_firmware && ui->st.link.state != WYE_LINK_LEGACY
                  ? "Device firmware is out of date - it updates itself over USB when idle."
                  : "Device firmware is out of date.";
        action = "Update";
        go = PAGE_DEVICE;
    } else if (ui->st.link.state == WYE_LINK_SEARCHING) {
        msg = "No Wye bridge found. Plug it in, or set up new hardware.";
        action = "Set up";
        go = PAGE_SETUP;
    }
    if (!msg) {
        return false;
    }
    Color col = go == PAGE_DEVICE && ui->st.fw_phase != WYE_FWS_IDLE ? UI_ACCENT : UI_WARN;
    DrawRectangleRounded(r, 0.4f, 8, Fade(col, 0.12f));
    DrawRectangleRounded((Rectangle){r.x, r.y + 6, 4, r.height - 12}, 1.0f, 4, col);
    BeginScissorMode((int)r.x, (int)r.y, (int)(r.width - 160), (int)r.height);
    ui_text(ui, msg, r.x + 18, r.y + (r.height - UI_FONT) / 2, UI_FONT, UI_TEXT);
    EndScissorMode();
    if (ui_button_tinted((Rectangle){r.x + r.width - 148, r.y + (r.height - 32) / 2, 136, 32}, action, col)) {
        char err[160] = "";
        if (go != PAGE_HOME) {
            ui->page = go;
        } else if (!plat_input_permission_request(err, sizeof(err))) {
            ui_toast(ui, err[0] ? err : "Permission not granted");
        } else {
            ui_toast(ui, "Access granted");
        }
    }
    return true;
}
