/* Requests from the engine and the OS: paste, show, notices, dropped files, closing. */
#include "ui.h"

#include "plat_sys.h"
#include "plat_window.h"

void ui_show_window(struct wye_ui *ui)
{
    ClearWindowState(FLAG_WINDOW_HIDDEN | FLAG_WINDOW_MINIMIZED);
    ui->hidden = false;
    if (ui->mini) {
        ui_mini_leave(ui);
    }
    plat_window_to_current_desktop(GetWindowHandle());
    SetWindowFocused();
}

/* Hides the window; the program keeps running and the tray brings it back. */
void ui_hide_window(struct wye_ui *ui)
{
    if (ui->mini) {
        ui_mini_leave(ui);
    }
    SetWindowState(FLAG_WINDOW_HIDDEN);
    ui->hidden = true;
}

void ui_handle_requests(struct wye_ui *ui)
{
    unsigned req = wye_engine_take_ui_requests();
    if (req & WYE_UI_REQ_PASTE) {
        const char *clip = GetClipboardText();
        if (clip && *clip) {
            wye_engine_type_text(clip);
        } else {
            ui_toast(ui, "Clipboard is empty");
        }
    }
    if (req & WYE_UI_REQ_MINI) {
        ui->mini ? ui_mini_leave(ui) : ui_mini_enter(ui);
    }
    if (req & WYE_UI_REQ_FOLDER) {
        ui_open_shared_folder(ui);
    }
    if ((req & WYE_UI_REQ_SHOW) || plat_instance_show_requested()) {
        ui_show_window(ui);
    }
    char notice[WYE_NOTICE_LEN];
    while (wye_engine_take_notice(notice, sizeof(notice))) {
        ui_toast(ui, notice);
    }
}

void ui_handle_close(struct wye_ui *ui)
{
    if (plat_instance_quit_requested()) {
        ui->quit = true;
    } else if (WindowShouldClose()) {
        ui_hide_window(ui); /* closing the window keeps Wye running in the tray; Quit ends it */
    }
}
