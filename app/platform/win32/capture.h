/* Windows capture internals shared by the hook procedures. */
#ifndef WYE_WIN32_CAPTURE_H
#define WYE_WIN32_CAPTURE_H

#include "plat_input.h"

#include <windows.h>

#define WM_WYE_FORWARDING (WM_APP + 1)

struct win_capture {
    wye_input_sink sink;
    void *ctx;
    DWORD thread_id;
    HANDLE thread, ready;
    HHOOK kbd_hook, mouse_hook;
    volatile LONG forwarding, pointer_local;
    volatile enum plat_input_status status;
    POINT lock, saved;            /* pointer parked here while forwarding */
    DWORD last_callback, last_input;
    BYTE down[256];               /* HID usages currently held (repeat detection) */
};

extern struct win_capture g_win_cap;

LRESULT CALLBACK win_keyboard_proc(int code, WPARAM wp, LPARAM lp);
LRESULT CALLBACK win_mouse_proc(int code, WPARAM wp, LPARAM lp);
void win_install_hooks(void);
void win_apply_forwarding(void);

/* keymap.c */
uint16_t win_scancode_to_hid(DWORD scan, bool extended, DWORD vk);
uint16_t win_vk_to_consumer(DWORD vk);
uint8_t win_vk_to_system(DWORD vk);

#endif
