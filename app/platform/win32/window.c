/* Windows window extras. */
#include "plat_window.h"
#include <windows.h>

void plat_window_all_desktops(void *handle, bool on)
{
    /* Pinning to all virtual desktops has no public API; topmost is the best available. */
    (void)handle;
    (void)on;
}

void plat_window_to_current_desktop(void *handle)
{
    if (handle) {
        ShowWindow((HWND)handle, SW_SHOW);
        SetForegroundWindow((HWND)handle); /* Windows opens it on the active virtual desktop */
    }
}

bool plat_cursor_screen_pos(void *handle, int *x, int *y)
{
    POINT p;
    (void)handle;
    if (!GetCursorPos(&p)) {
        return false;
    }
    *x = p.x;
    *y = p.y;
    return true;
}

void plat_window_set_overlay(void *handle, bool on)
{
    /* A tool window has no taskbar or Alt+Tab entry; no-activate keeps the focus where it is.
       Windows reads these when the window is shown. */
    HWND w = (HWND)handle;
    LONG_PTR ex = GetWindowLongPtrA(w, GWL_EXSTYLE);
    ex = on ? (ex | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE) & ~(LONG_PTR)WS_EX_APPWINDOW
            : (ex & ~(LONG_PTR)(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE)) | WS_EX_APPWINDOW;
    SetWindowLongPtrA(w, GWL_EXSTYLE, ex);
}
