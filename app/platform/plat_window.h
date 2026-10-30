/*
 * Native window extras raylib does not offer. Each takes raylib's
 * GetWindowHandle() (a GLFWwindow* on Linux, an HWND on Windows).
 */
#ifndef WYE_PLAT_WINDOW_H
#define WYE_PLAT_WINDOW_H

#include <stdbool.h>

/* Show the window on every workspace / virtual desktop (X11: sticky). */
void plat_window_all_desktops(void *handle, bool on);
/* Move the window to the workspace the user is on now and bring it to the front. */
void plat_window_to_current_desktop(void *handle);
/* Pointer position in screen coordinates (window-independent, for dragging). */
bool plat_cursor_screen_pos(void *handle, int *x, int *y);

#endif
