/* Low-level keyboard and mouse hook procedures. Must return quickly. */
#include "capture.h"

#include "wye_proto.h"

static bool dispatch(struct wye_input_event *ev)
{
    struct win_capture *c = &g_win_cap;
    c->last_callback = GetTickCount();
    return c->sink(c->ctx, ev);
}

LRESULT CALLBACK win_keyboard_proc(int code, WPARAM wp, LPARAM lp)
{
    const KBDLLHOOKSTRUCT *k = (const KBDLLHOOKSTRUCT *)lp;
    /* Skip injected input and the phantom Ctrl that AltGr generates. */
    if (code != HC_ACTION || (k->flags & LLKHF_INJECTED) || (k->scanCode & 0x200)) {
        return CallNextHookEx(NULL, code, wp, lp);
    }
    struct wye_input_event ev = {0};
    ev.down = !(k->flags & LLKHF_UP);
    if ((ev.code = win_vk_to_consumer(k->vkCode))) {
        ev.type = WYE_EV_CONSUMER;
    } else if ((ev.code = win_vk_to_system(k->vkCode))) {
        ev.type = WYE_EV_SYSTEM;
    } else if ((ev.code = win_scancode_to_hid(k->scanCode, (k->flags & LLKHF_EXTENDED) != 0, k->vkCode))) {
        ev.type = WYE_EV_KEY;
        BYTE *held = &g_win_cap.down[ev.code & 0xff];
        ev.repeat = ev.down && *held;
        *held = ev.down;
    }
    bool swallow = ev.type ? dispatch(&ev) : g_win_cap.forwarding != 0;
    return swallow ? 1 : CallNextHookEx(NULL, code, wp, lp);
}

static bool button_event(WPARAM wp, DWORD data, struct wye_input_event *ev)
{
    ev->type = WYE_EV_BUTTON;
    switch (wp) {
    case WM_LBUTTONDOWN: case WM_LBUTTONUP: ev->code = WYE_BTN_LEFT; break;
    case WM_RBUTTONDOWN: case WM_RBUTTONUP: ev->code = WYE_BTN_RIGHT; break;
    case WM_MBUTTONDOWN: case WM_MBUTTONUP: ev->code = WYE_BTN_MIDDLE; break;
    case WM_XBUTTONDOWN: case WM_XBUTTONUP:
        ev->code = HIWORD(data) == XBUTTON1 ? WYE_BTN_BACK : WYE_BTN_FORWARD;
        break;
    default: return false;
    }
    ev->down = wp == WM_LBUTTONDOWN || wp == WM_RBUTTONDOWN || wp == WM_MBUTTONDOWN || wp == WM_XBUTTONDOWN;
    return true;
}

LRESULT CALLBACK win_mouse_proc(int code, WPARAM wp, LPARAM lp)
{
    const MSLLHOOKSTRUCT *m = (const MSLLHOOKSTRUCT *)lp;
    struct win_capture *c = &g_win_cap;
    if (code != HC_ACTION || (m->flags & LLMHF_INJECTED)) {
        return CallNextHookEx(NULL, code, wp, lp);
    }
    struct wye_input_event ev = {0};
    if (wp == WM_MOUSEMOVE) {
        if (!c->forwarding || c->pointer_local) {
            return CallNextHookEx(NULL, code, wp, lp);
        }
        /* The cursor stays parked at c->lock; pt is where it would have gone. */
        ev.type = WYE_EV_MOVE;
        ev.dx = m->pt.x - c->lock.x;
        ev.dy = m->pt.y - c->lock.y;
        return (ev.dx || ev.dy) && !dispatch(&ev) ? CallNextHookEx(NULL, code, wp, lp) : 1;
    }
    if (wp == WM_MOUSEWHEEL || wp == WM_MOUSEHWHEEL) {
        ev.type = WYE_EV_WHEEL;
        short delta = (short)HIWORD(m->mouseData);
        ev.dy = wp == WM_MOUSEWHEEL ? delta : 0;
        ev.dx = wp == WM_MOUSEHWHEEL ? delta : 0;
    } else if (!button_event(wp, m->mouseData, &ev)) {
        return CallNextHookEx(NULL, code, wp, lp);
    }
    return dispatch(&ev) ? 1 : CallNextHookEx(NULL, code, wp, lp);
}

/* Runs on the hook thread when forwarding toggles. */
void win_apply_forwarding(void)
{
    struct win_capture *c = &g_win_cap;
    if (c->forwarding && !c->pointer_local) {
        GetCursorPos(&c->saved);
        HMONITOR mon = MonitorFromPoint(c->saved, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = {.cbSize = sizeof(mi)};
        GetMonitorInfoA(mon, &mi);
        c->lock.x = (mi.rcMonitor.left + mi.rcMonitor.right) / 2; /* far from edges: no clipping */
        c->lock.y = (mi.rcMonitor.top + mi.rcMonitor.bottom) / 2;
        SetCursorPos(c->lock.x, c->lock.y);
    } else if (c->lock.x || c->lock.y) {
        SetCursorPos(c->saved.x, c->saved.y);
        c->lock = (POINT){0, 0};
    }
}
