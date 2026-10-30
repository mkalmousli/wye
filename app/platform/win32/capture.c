/* plat_input on Windows: hook thread, message loop and hook health. */
#include "capture.h"

#include <stdio.h>

struct win_capture g_win_cap;

void win_install_hooks(void)
{
    struct win_capture *c = &g_win_cap;
    if (c->kbd_hook) UnhookWindowsHookEx(c->kbd_hook);
    if (c->mouse_hook) UnhookWindowsHookEx(c->mouse_hook);
    HINSTANCE mod = GetModuleHandleA(NULL);
    c->kbd_hook = SetWindowsHookExA(WH_KEYBOARD_LL, win_keyboard_proc, mod, 0);
    c->mouse_hook = SetWindowsHookExA(WH_MOUSE_LL, win_mouse_proc, mod, 0);
    c->status = c->kbd_hook && c->mouse_hook ? PLAT_INPUT_RUNNING : PLAT_INPUT_FAILED;
    c->last_callback = GetTickCount();
}

/* Windows silently drops hooks that time out; detect that and reinstall. */
static void check_health(struct win_capture *c)
{
    LASTINPUTINFO li = {.cbSize = sizeof(li)};
    if (GetLastInputInfo(&li) && li.dwTime != c->last_input) {
        bool missed = (LONG)(li.dwTime - c->last_callback) > 2000;
        c->last_input = li.dwTime;
        if (missed) {
            win_install_hooks();
        }
    }
}

static DWORD WINAPI hook_thread(LPVOID arg)
{
    struct win_capture *c = arg;
    MSG msg;
    PeekMessageA(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE); /* create the queue */
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    win_install_hooks();
    UINT_PTR timer = SetTimer(NULL, 0, 1000, NULL);
    SetEvent(c->ready);
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        if (msg.message == WM_TIMER) {
            check_health(c);
        } else if (msg.message == WM_WYE_FORWARDING) {
            win_apply_forwarding();
        }
    }
    KillTimer(NULL, timer);
    UnhookWindowsHookEx(c->kbd_hook);
    UnhookWindowsHookEx(c->mouse_hook);
    c->kbd_hook = c->mouse_hook = NULL;
    return 0;
}

bool plat_input_start(wye_input_sink sink, void *ctx, char *err, size_t errlen)
{
    struct win_capture *c = &g_win_cap;
    c->sink = sink;
    c->ctx = ctx;
    c->ready = CreateEventA(NULL, TRUE, FALSE, NULL);
    c->thread = CreateThread(NULL, 0, hook_thread, c, 0, &c->thread_id);
    WaitForSingleObject(c->ready, 5000);
    CloseHandle(c->ready);
    if (c->status != PLAT_INPUT_RUNNING) {
        snprintf(err, errlen, "could not install keyboard/mouse hooks (error %lu)", GetLastError());
        return false;
    }
    return true;
}

void plat_input_stop(void)
{
    struct win_capture *c = &g_win_cap;
    if (!c->thread) {
        return;
    }
    PostThreadMessageA(c->thread_id, WM_QUIT, 0, 0);
    WaitForSingleObject(c->thread, 5000);
    CloseHandle(c->thread);
    c->thread = NULL;
    c->status = PLAT_INPUT_STOPPED;
}

enum plat_input_status plat_input_status(void) { return g_win_cap.status; }

void plat_input_set_forwarding(bool forwarding)
{
    InterlockedExchange(&g_win_cap.forwarding, forwarding ? 1 : 0);
    if (g_win_cap.thread) {
        PostThreadMessageA(g_win_cap.thread_id, WM_WYE_FORWARDING, 0, 0);
    }
}

void plat_input_set_pointer_local(bool local)
{
    InterlockedExchange(&g_win_cap.pointer_local, local ? 1 : 0); /* read when forwarding (re)starts */
}

void plat_input_set_leds(uint8_t leds) { (void)leds; } /* Windows cannot drive keyboard LEDs directly */
