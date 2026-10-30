/* The system tray: an icon that shows Wye's state at a glance, and a menu of quick actions. */
#include "ui.h"
#include "ui_theme.h"

#include "plat_sys.h"
#include "plat_tray.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MENU_W 250
#define REFRESH_S 0.25

extern const unsigned char wye_icon_png[];
extern const size_t wye_icon_png_len;

enum tray_id { ID_STATUS = 1, ID_TOGGLE, ID_OPEN, ID_MINI, ID_JIGGLER, ID_PASTE, ID_CAD, ID_FOLDER, ID_QUIT };
enum tray_look { LOOK_NONE = -1, LOOK_OFFLINE, LOOK_PARTLY, LOOK_READY, LOOK_CONTROLLING };

static struct {
    bool started;
    double refreshed, retry_at;
    enum tray_look look;
    int icon_size;
    char tooltip[PLAT_TRAY_LABEL];
    struct plat_tray_item items[PLAT_TRAY_ITEMS];
    int item_count;
    char title[PLAT_TRAY_LABEL];
} g = {.look = LOOK_NONE};

/* Menu rows, painted for systems whose tray has no menus of its own. */
static uint8_t *paint(const struct plat_tray_item *items, int count, int hover, int *w, int *h)
{
    int height = 0;
    for (int i = 0; i < count; ++i) {
        height += items[i].label[0] ? PLAT_TRAY_ROW_H : PLAT_TRAY_SEPARATOR_H;
    }
    Font font = ui_font(UI_FONT_SMALL, false);
    float size = (float)font.baseSize;
    int width = MENU_W;
    if (count == 1) { /* a tooltip fits its text */
        width = (int)MeasureTextEx(font, items[0].label, size, 0).x + 20;
    }
    Image img = GenImageColor(width, height, UI_CARD);
    int y = 0;
    for (int i = 0; i < count; ++i) {
        if (!items[i].label[0]) {
            ImageDrawRectangle(&img, 8, y + PLAT_TRAY_SEPARATOR_H / 2, width - 16, 1, UI_CARD_EDGE);
            y += PLAT_TRAY_SEPARATOR_H;
            continue;
        }
        if (i == hover) {
            ImageDrawRectangle(&img, 3, y + 2, width - 6, PLAT_TRAY_ROW_H - 4, UI_ACCENT);
        }
        Color col = !items[i].enabled ? UI_MUTED : i == hover ? WHITE : UI_TEXT;
        float x = count == 1 ? 10 : 30;
        float text_y = (float)y + ((float)PLAT_TRAY_ROW_H - size) / 2;
        if (items[i].checked) {
            ImageDrawCircle(&img, 15, y + PLAT_TRAY_ROW_H / 2, 4, i == hover ? WHITE : UI_OK);
        }
        ImageDrawTextEx(&img, font, items[i].label, (Vector2){x, text_y}, size, 0, col);
        y += PLAT_TRAY_ROW_H;
    }
    ImageDrawRectangleLines(&img, (Rectangle){0, 0, (float)width, (float)height}, 1, UI_CARD_EDGE);
    *w = width;
    *h = height;
    return img.data; /* the caller frees it */
}

static enum tray_look look_now(const struct wye_ui *ui)
{
    if (ui->st.mode != WYE_FWD_OFF) {
        return LOOK_CONTROLLING;
    }
    return ui->st.link.state == WYE_LINK_READY ? LOOK_READY : ui->st.link.state == WYE_LINK_BRIDGE ? LOOK_PARTLY : LOOK_OFFLINE;
}

/* The logo, greyed while nothing is connected, with a dot in the state's colour. */
static void update_icon(enum tray_look look, int size)
{
    Image img = LoadImageFromMemory(".png", wye_icon_png, (int)wye_icon_png_len);
    if (!img.data) {
        return;
    }
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    ImageResize(&img, size, size);
    if (look == LOOK_OFFLINE) {
        ImageColorGrayscale(&img);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    }
    Color dot = look == LOOK_CONTROLLING ? UI_TARGET : look == LOOK_READY ? UI_OK : look == LOOK_PARTLY ? UI_WARN : UI_BAD;
    int r = size / 4 < 3 ? 3 : size / 4, at = size - r - 1;
    ImageDrawCircle(&img, at, at, r + 1, (Color){13, 19, 32, 255});
    ImageDrawCircle(&img, at, at, r, dot);
    plat_tray_set_icon(img.data, size);
    UnloadImage(img);
}

static void set_item(int *n, int id, const char *label, bool enabled, bool checked)
{
    struct plat_tray_item *it = &g.items[(*n)++];
    memset(it, 0, sizeof(*it));
    it->id = id;
    it->enabled = enabled;
    it->checked = checked;
    snprintf(it->label, sizeof(it->label), "%s", label);
}

static void update_menu(struct wye_ui *ui, const char *status)
{
    static struct plat_tray_item shown[PLAT_TRAY_ITEMS];
    static int shown_count = -1;
    bool fwd = ui->st.mode != WYE_FWD_OFF, ready = ui->st.link.state == WYE_LINK_READY;
    int n = 0;
    memset(g.items, 0, sizeof(g.items));
    set_item(&n, ID_STATUS, TextFormat("Wye: %s", status), false, false);
    set_item(&n, 0, "", false, false);
    set_item(&n, ID_TOGGLE, TextFormat("%s (%s)", fwd ? "Back to this computer" : "Control the other computer",
                                       ui_key_name(ui->cfg.hotkey)), fwd || ready, false);
    set_item(&n, ID_OPEN, "Open Wye", true, false);
    set_item(&n, ID_MINI, "Mini panel", true, ui->mini);
    set_item(&n, 0, "", false, false);
    set_item(&n, ID_JIGGLER, "Jiggler", true, ui->st.jiggler_on);
    set_item(&n, ID_PASTE, "Type the clipboard", ready, false);
    set_item(&n, ID_CAD, "Send Ctrl+Alt+Del", ready, false);
    set_item(&n, ID_FOLDER, "Open the shared folder", ui->st.drive.enabled, false);
    set_item(&n, 0, "", false, false);
    set_item(&n, ID_QUIT, "Quit Wye", true, false);
    if (n != shown_count || memcmp(shown, g.items, sizeof(shown)) != 0) {
        memcpy(shown, g.items, sizeof(shown));
        shown_count = n;
        g.item_count = n;
        plat_tray_set_menu(g.items, n);
    }
}

static void update(struct wye_ui *ui)
{
    Color c;
    const char *status = ui_status_text(ui, &c);
    enum tray_look look = look_now(ui);
    int size = plat_tray_icon_size();
    if (look != g.look || size != g.icon_size) {
        g.look = look;
        g.icon_size = size;
        update_icon(look, size);
    }
    char tip[PLAT_TRAY_LABEL];
    snprintf(tip, sizeof(tip), "Wye: %s", status);
    if (strcmp(tip, g.tooltip) != 0) {
        memcpy(g.tooltip, tip, sizeof(tip));
        plat_tray_set_tooltip(tip);
    }
    update_menu(ui, status);
}

static void act(struct wye_ui *ui, int id)
{
    const char *clip;
    switch (id) {
    case PLAT_TRAY_ACTIVATE:
    case ID_OPEN: ui_show_window(ui); break;
    case ID_TOGGLE: wye_engine_toggle(); break;
    case ID_MINI:
        if (ui->hidden && !ui->mini) {
            ui->hidden = false; /* entering shows it */
        }
        ui->mini ? ui_mini_leave(ui) : ui_mini_enter(ui);
        break;
    case ID_JIGGLER: wye_engine_jiggler(!ui->st.jiggler_on); break;
    case ID_PASTE:
        clip = GetClipboardText();
        wye_engine_type_text(clip ? clip : "");
        break;
    case ID_CAD: wye_engine_ctrl_alt_del(); break;
    case ID_FOLDER: ui_open_shared_folder(ui); break;
    case ID_QUIT: ui->quit = true; break;
    default: break;
    }
}

void ui_tray_stop(void)
{
    if (g.started) {
        plat_tray_stop();
        g.started = false;
        g.look = LOOK_NONE;
        g.icon_size = 0;
        g.tooltip[0] = '\0';
    }
}

bool ui_tray_showing(void)
{
    return g.started && plat_tray_available();
}

void ui_tray_tick(struct wye_ui *ui)
{
    if (ui->cfg.tray_enabled != g.started) {
        if (g.started) {
            ui_tray_stop();
        } else if (GetTime() >= g.retry_at) {
            g.retry_at = GetTime() + 5.0;
            g.started = plat_tray_start(paint);
        }
    }
    if (!g.started) {
        return;
    }
    if (GetTime() - g.refreshed > REFRESH_S) {
        g.refreshed = GetTime();
        update(ui);
    }
    for (int id = plat_tray_poll(); id != PLAT_TRAY_NONE; id = plat_tray_poll()) {
        act(ui, id);
    }
}

/* The window's title says it too, for taskbars. */
void ui_update_title(struct wye_ui *ui)
{
    Color c;
    char title[PLAT_TRAY_LABEL];
    snprintf(title, sizeof(title), "Wye - %s", ui_status_text(ui, &c));
    if (strcmp(title, g.title) != 0) {
        memcpy(g.title, title, sizeof(title));
        SetWindowTitle(title);
    }
}
