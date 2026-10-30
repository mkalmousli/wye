/* About: version, diagnostics log, support links, quit. */
#include "ui.h"
#include "ui_theme.h"

#include "log.h"
#include "plat_sys.h"
#include "product.h"
#include "util/util.h"

#include <stdio.h>
#include <string.h>

static char g_lines[64][WYE_LOG_LINE];

static void copy_diagnostics(struct wye_ui *ui)
{
    static char buf[64 * WYE_LOG_LINE + 512];
    char os[160];
    plat_os_description(os, sizeof(os));
    int n = wye_log_snapshot(g_lines, 64);
    size_t used = (size_t)snprintf(buf, sizeof(buf), "%s %s on %s\n", WYE_PRODUCT_NAME, WYE_VERSION_STRING, os);
    for (int i = 0; i < n && used < sizeof(buf); ++i) {
        used += (size_t)snprintf(buf + used, sizeof(buf) - used, "%s\n", g_lines[i]);
    }
    SetClipboardText(buf);
    ui_toast(ui, "Diagnostics copied - paste them into your support request");
}

static void log_card(struct wye_ui *ui, Rectangle r)
{
    Rectangle c = ui_card(ui, r, "Activity log");
    int rows = (int)(c.height / 20);
    int n = wye_log_snapshot(g_lines, rows < 64 ? rows : 64);
    BeginScissorMode((int)c.x, (int)c.y, (int)c.width, (int)c.height);
    for (int i = 0; i < n; ++i) {
        Color col = strstr(g_lines[i], " error ") ? UI_BAD : strstr(g_lines[i], " warn ") ? UI_WARN : UI_MUTED;
        ui_text(ui, g_lines[i], c.x, c.y + (float)i * 20, UI_FONT_SMALL, col);
    }
    EndScissorMode();
}

void page_about(struct wye_ui *ui, Rectangle a)
{
    char os[160];
    ui_text_bold(ui, WYE_PRODUCT_NAME, a.x, a.y, UI_FONT_TITLE, UI_TEXT);
    plat_os_description(os, sizeof(os));
    ui_text(ui, TextFormat("Version %s  -  %s", WYE_VERSION_STRING, os), a.x, a.y + 34, UI_FONT_SMALL, UI_MUTED);
    float y = a.y + 64, w = 140;
    if (ui_button((Rectangle){a.x, y, w, UI_ROW}, "Copy diagnostics")) {
        copy_diagnostics(ui);
    }
    if (ui_button((Rectangle){a.x + w + 8, y, w, UI_ROW}, "Open log folder")) {
        char dir[PLAT_PATH_MAX];
        if (plat_data_dir(dir, sizeof(dir))) {
            plat_reveal_path(dir);
        }
    }
    if (ui_button((Rectangle){a.x + 2 * (w + 8), y, w, UI_ROW}, "Get help")) {
        plat_open_url(WYE_SUPPORT_URL);
    }
    if (ui_button((Rectangle){a.x + 3 * (w + 8), y, w, UI_ROW}, "Source code")) {
        plat_open_url(WYE_SOURCE_URL);
    }
    if (ui_button((Rectangle){a.x + a.width - w, y, w, UI_ROW}, "Quit Wye")) {
        ui->quit = true;
    }
    log_card(ui, (Rectangle){a.x, y + 52, a.width, a.y + a.height - y - 112});
    ui_text(ui, "Free software under the GNU GPL, version 3 or later. You may use, study, change and share it.",
            a.x, a.y + a.height - 42, UI_FONT_SMALL, UI_MUTED);
    ui_text(ui, "Uses raylib, raygui (zlib), Monocypher (CC0/BSD), Inter (OFL), miniaudio (MIT-0), Pico SDK (BSD), TinyUSB (MIT).",
            a.x, a.y + a.height - 22, UI_FONT_SMALL, UI_MUTED);
}
