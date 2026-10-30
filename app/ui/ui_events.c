/* Requests from the engine and the OS: paste, show, notices, dropped files, closing. */
#include "ui.h"

#include "plat_sys.h"
#include "plat_window.h"

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
        ClearWindowState(FLAG_WINDOW_HIDDEN | FLAG_WINDOW_MINIMIZED);
        ui->hidden = false;
        if (ui->mini) ui_mini_leave(ui);
        ui->mini_for_control = false;
        plat_window_to_current_desktop(GetWindowHandle());
        SetWindowFocused();
    }
    char notice[WYE_NOTICE_LEN];
    while (wye_engine_take_notice(notice, sizeof(notice))) {
        ui_toast(ui, notice);
    }
}

void ui_handle_close(struct wye_ui *ui)
{
    if (WindowShouldClose() || plat_instance_quit_requested()) {
        ui->quit = true; /* closing the window ends the program, not just the window */
    }
}
