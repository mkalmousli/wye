/* Routines: one recorded event, in words. */
#include "routines.h"

#include "wye_codec.h"
#include "wye_proto.h"

#include <stdio.h>
#include <string.h>

static void buttons_text(uint8_t b, char *out, size_t cap)
{
    static const char *const NAMES[] = {"left", "right", "middle", "back", "forward"};
    size_t n = 0;
    out[0] = '\0';
    for (int i = 0; i < 5; ++i) {
        if (b & (1u << i)) {
            n += (size_t)snprintf(out + n, cap - n, "%s%s", n ? "+" : "", NAMES[i]);
        }
    }
}

static const char *keyboard(const struct wye_keyboard *k)
{
    static const char *const MODS[] = {"Ctrl", "Shift", "Alt", "Win", "RCtrl", "RShift", "AltGr", "RWin"};
    static char out[128];
    size_t n = (size_t)snprintf(out, sizeof(out), "Keys: ");
    bool any = false;
    for (int i = 0; i < 8; ++i) {
        if (k->modifiers & (1u << i)) {
            n += (size_t)snprintf(out + n, sizeof(out) - n, "%s%s", any ? " + " : "", MODS[i]);
            any = true;
        }
    }
    for (int i = 0; i < 6; ++i) {
        if (k->keys[i]) {
            n += (size_t)snprintf(out + n, sizeof(out) - n, "%s%s", any ? " + " : "", ui_key_name(k->keys[i]));
            any = true;
        }
    }
    return any ? out : "All keys released";
}

const char *routines_event_text(const struct wye_routine_event *ev)
{
    static char out[128];
    char held[48];
    struct wye_keyboard k;
    struct wye_mouse m;
    struct wye_abs_mouse a;
    switch (ev->type) {
    case WYE_MSG_KEYBOARD:
        return wye_keyboard_unpack(ev->payload, ev->len, &k) ? keyboard(&k) : "Keys (damaged)";
    case WYE_MSG_MOUSE:
        if (!wye_mouse_unpack(ev->payload, ev->len, &m)) {
            return "Mouse (damaged)";
        }
        buttons_text(m.buttons, held, sizeof(held));
        if (m.dx || m.dy) {
            snprintf(out, sizeof(out), "Move %+d, %+d%s%s", m.dx, m.dy, held[0] ? "  holding " : "", held);
        } else if (m.wheel || m.pan) {
            snprintf(out, sizeof(out), "Scroll %+d%s", m.wheel ? m.wheel : m.pan, m.wheel ? "" : " sideways");
        } else {
            snprintf(out, sizeof(out), held[0] ? "Press %s" : "Buttons released", held);
        }
        return out;
    case WYE_MSG_ABS_MOUSE:
        if (!wye_abs_mouse_unpack(ev->payload, ev->len, &a)) {
            return "Mouse (damaged)";
        }
        snprintf(out, sizeof(out), "Move to %u, %u", a.x, a.y);
        return out;
    case WYE_MSG_CONSUMER:
        snprintf(out, sizeof(out), ev->len >= 2 && (ev->payload[0] || ev->payload[1]) ? "Media key %02x%02x" : "Media key released",
                 ev->len >= 2 ? ev->payload[1] : 0, ev->len >= 2 ? ev->payload[0] : 0);
        return out;
    case WYE_MSG_SYSTEM:
        return ev->len && ev->payload[0] ? "Power key" : "Power key released";
    case WYE_MSG_RELEASE_ALL:
        return "Release everything";
    default:
        snprintf(out, sizeof(out), "Unknown event %u", ev->type);
        return out;
    }
}
