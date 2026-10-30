/* System tray on Windows: a notification icon and a native menu. */
#include "plat_tray.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <shellapi.h> /* needs windows.h first */

#define WM_TRAY (WM_APP + 1)
#define ICON_ID 1
#define MENU_ID_BASE 100
#define QUEUE 8

static struct {
    HWND hwnd;
    HICON icon;
    UINT taskbar_created;
    bool added;
    char tooltip[PLAT_TRAY_LABEL];
    struct plat_tray_item items[PLAT_TRAY_ITEMS];
    int item_count;
    int queue[QUEUE], queued;
} g;

static void push(int id)
{
    if (g.queued < QUEUE) {
        g.queue[g.queued++] = id;
    }
}

static void fill(NOTIFYICONDATAA *n)
{
    memset(n, 0, sizeof(*n));
    n->cbSize = sizeof(*n);
    n->hWnd = g.hwnd;
    n->uID = ICON_ID;
    n->uFlags = NIF_MESSAGE | NIF_TIP | (g.icon ? NIF_ICON : 0);
    n->uCallbackMessage = WM_TRAY;
    n->hIcon = g.icon;
    snprintf(n->szTip, sizeof(n->szTip), "%s", g.tooltip);
}

static void add_or_update(void)
{
    NOTIFYICONDATAA n;
    fill(&n);
    if (g.added) {
        Shell_NotifyIconA(NIM_MODIFY, &n);
    } else {
        g.added = Shell_NotifyIconA(NIM_ADD, &n) != FALSE;
    }
}

static void show_menu(void)
{
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }
    for (int i = 0; i < g.item_count; ++i) {
        const struct plat_tray_item *it = &g.items[i];
        if (!it->label[0]) {
            AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
        } else {
            UINT flags = MF_STRING | (it->enabled ? MF_ENABLED : MF_GRAYED) | (it->checked ? MF_CHECKED : MF_UNCHECKED);
            AppendMenuA(menu, flags, (UINT_PTR)(MENU_ID_BASE + i), it->label);
        }
    }
    POINT at;
    GetCursorPos(&at);
    SetForegroundWindow(g.hwnd); /* otherwise the menu stays up after a click elsewhere */
    int chosen = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, at.x, at.y, 0, g.hwnd, NULL);
    PostMessageA(g.hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
    if (chosen >= MENU_ID_BASE && chosen - MENU_ID_BASE < g.item_count) {
        push(g.items[chosen - MENU_ID_BASE].id);
    }
}

static LRESULT CALLBACK tray_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_TRAY) {
        if (lp == WM_LBUTTONUP) {
            push(PLAT_TRAY_ACTIVATE);
        } else if (lp == WM_RBUTTONUP) {
            show_menu();
        }
        return 0;
    }
    if (g.taskbar_created && msg == g.taskbar_created) { /* Explorer restarted */
        g.added = false;
        add_or_update();
        return 0;
    }
    return DefWindowProcA(w, msg, wp, lp);
}

bool plat_tray_start(plat_tray_painter painter)
{
    (void)painter; /* Windows draws its own menus */
    if (g.hwnd) {
        return true;
    }
    WNDCLASSA c;
    memset(&c, 0, sizeof(c));
    c.lpfnWndProc = tray_proc;
    c.hInstance = GetModuleHandleA(NULL);
    c.lpszClassName = "WyeTray";
    RegisterClassA(&c); /* fails harmlessly if it already exists */
    g.hwnd = CreateWindowA("WyeTray", "Wye", 0, 0, 0, 0, 0, NULL, NULL, c.hInstance, NULL); /* top-level: gets broadcasts */
    if (!g.hwnd) {
        return false;
    }
    g.taskbar_created = RegisterWindowMessageA("TaskbarCreated");
    add_or_update();
    return true;
}

void plat_tray_stop(void)
{
    if (g.hwnd) {
        NOTIFYICONDATAA n;
        fill(&n);
        Shell_NotifyIconA(NIM_DELETE, &n);
        DestroyWindow(g.hwnd);
    }
    if (g.icon) {
        DestroyIcon(g.icon);
    }
    memset(&g, 0, sizeof(g));
}

bool plat_tray_available(void)
{
    return g.hwnd && g.added;
}

int plat_tray_icon_size(void)
{
    return GetSystemMetrics(SM_CXSMICON);
}

void plat_tray_set_icon(const uint8_t *rgba, int size)
{
    BITMAPV5HEADER h;
    memset(&h, 0, sizeof(h));
    h.bV5Size = sizeof(h);
    h.bV5Width = size;
    h.bV5Height = -size; /* top-down, like the picture */
    h.bV5Planes = 1;
    h.bV5BitCount = 32;
    h.bV5Compression = BI_BITFIELDS;
    h.bV5RedMask = 0x00ff0000;
    h.bV5GreenMask = 0x0000ff00;
    h.bV5BlueMask = 0x000000ff;
    h.bV5AlphaMask = 0xff000000;
    void *bits = NULL;
    HDC dc = GetDC(NULL);
    HBITMAP color = CreateDIBSection(dc, (BITMAPINFO *)&h, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(NULL, dc);
    HBITMAP mask = CreateBitmap(size, size, 1, 1, NULL);
    if (color && mask && bits) {
        uint8_t *out = bits;
        for (int i = 0; i < size * size; ++i) { /* RGBA to BGRA */
            out[i * 4] = rgba[i * 4 + 2];
            out[i * 4 + 1] = rgba[i * 4 + 1];
            out[i * 4 + 2] = rgba[i * 4];
            out[i * 4 + 3] = rgba[i * 4 + 3];
        }
        ICONINFO info = {TRUE, 0, 0, mask, color};
        HICON icon = CreateIconIndirect(&info);
        if (icon) {
            if (g.icon) {
                DestroyIcon(g.icon);
            }
            g.icon = icon;
            add_or_update();
        }
    }
    if (color) {
        DeleteObject(color);
    }
    if (mask) {
        DeleteObject(mask);
    }
}

void plat_tray_set_tooltip(const char *text)
{
    snprintf(g.tooltip, sizeof(g.tooltip), "%s", text);
    if (g.hwnd) {
        add_or_update();
    }
}

void plat_tray_set_menu(const struct plat_tray_item *items, int count)
{
    g.item_count = count > PLAT_TRAY_ITEMS ? PLAT_TRAY_ITEMS : count;
    memcpy(g.items, items, (size_t)g.item_count * sizeof(items[0]));
}

int plat_tray_poll(void)
{
    /* Its messages arrive through the same message loop as the main window's. */
    if (!g.queued) {
        return PLAT_TRAY_NONE;
    }
    int id = g.queue[0];
    memmove(g.queue, g.queue + 1, (size_t)--g.queued * sizeof(g.queue[0]));
    return id;
}
