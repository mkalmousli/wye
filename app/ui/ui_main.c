/* Window lifecycle and the per-frame loop. */
#include "ui.h"
#include "ui_fx.h"
#include "ui_theme.h"

#include "plat_sys.h"
#include "product.h"
#include "raygui.h"

#include <string.h>

void ui_sidebar(struct wye_ui *ui);
void ui_handle_requests(struct wye_ui *ui);
void ui_handle_close(struct wye_ui *ui);

static void (*const PAGES[PAGE_COUNT])(struct wye_ui *, Rectangle) = {
    page_home, page_routines, page_devices, page_device, page_shortcuts, page_settings,
    page_about, page_setup, page_identity,
};

static void frame(struct wye_ui *ui)
{
    wye_engine_status(&ui->st);
    wye_updater_status(&ui->upd);
    wye_updater_set_busy(ui->st.mode != WYE_FWD_OFF || ui->st.playing || ui->st.fw_phase != WYE_FWS_IDLE);
    if (ui->upd.state == WYE_UPD_RESTARTING) {
        ui->quit = true;
    }
    ui_handle_requests(ui);
    if (ui->page != PAGE_SHORTCUTS) {
        shortcuts_stop_learning();
    }
    ui_mini_follow_control(ui);
    BeginDrawing();
    ui_background();
    if (ui->mini) {
        ui_mini_panel(ui);
    } else {
        ui_sidebar(ui);
        float x = UI_SIDEBAR_W + UI_PAD;
        Rectangle area = {x, UI_PAD, (float)GetScreenWidth() - x - UI_PAD, (float)GetScreenHeight() - 2 * UI_PAD};
        PAGES[ui->page](ui, area);
        ui_draw_toasts(ui);
    }
    EndDrawing();
    ui_handle_close(ui);
}

extern const unsigned char wye_icon_png[];
extern const size_t wye_icon_png_len;

/* The title bar, taskbar and task switcher take the icon from the window itself. */
static void set_window_icon(void)
{
    Image icon = LoadImageFromMemory(".png", wye_icon_png, (int)wye_icon_png_len);
    if (icon.data) {
        ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8); /* all SetWindowIcon accepts */
        SetWindowIcon(icon);
        UnloadImage(icon);
    }
}

int wye_ui_run(struct wye_ui *ui)
{
    /* ALWAYS_RUN: raylib would otherwise stop the loop while minimized, and a request to
       come to the front, a shortcut or a signal to quit would go unseen. */
    unsigned flags = FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT |
                     FLAG_WINDOW_ALWAYS_RUN;
    if (ui->hidden) {
        flags |= FLAG_WINDOW_HIDDEN;
    }
    SetConfigFlags(flags);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1000, 660, WYE_PRODUCT_NAME);
    SetWindowMinSize(860, 580);
    SetExitKey(KEY_NULL);
    set_window_icon();
    ui_theme_load(ui);
    ui_mini_restore_position(ui);
    if (ui->cfg.last_mini && ui->page != PAGE_SETUP) {
        ui_mini_enter(ui); /* as it was when Wye last ran */
    }
    while (!ui->quit) {
        SetTargetFPS(ui->hidden || IsWindowMinimized() ? 10 : 60);
        frame(ui);
    }
    ui_theme_unload(ui);
    CloseWindow();
    return 0;
}
