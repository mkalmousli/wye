/* Keyboard routing, hotkey and chords. */
#include "input.h"

#include "hid_usage.h"
#include "wye_proto.h"

void wye_input_default_chords(uint8_t keys[WYE_ACT_COUNT])
{
    for (int a = 0; a < WYE_ACT_COUNT; ++a) {
        keys[a] = 0;
    }
    keys[WYE_ACT_PASTE] = HID_KEY_V;
    keys[WYE_ACT_JIGGLER] = HID_KEY_J;
    keys[WYE_ACT_CTRL_ALT_DEL] = HID_KEY_DELETE;
    keys[WYE_ACT_SHOW_WINDOW] = HID_KEY_O;
    keys[WYE_ACT_RELEASE] = HID_KEY_ESCAPE;
    keys[WYE_ACT_MINI] = HID_KEY_M;
    keys[WYE_ACT_FOLDER] = HID_KEY_F;
}

bool wye_shortcut_key_accepted(uint16_t key, uint16_t hotkey)
{
    return key != hotkey && !HID_IS_MODIFIER(key);
}

int wye_shortcut_assign(uint8_t *keys, int count, int action, uint8_t key)
{
    int lost = 0;
    for (int a = 0; key && a < count; ++a) {
        if (a != action && keys[a] == key) {
            keys[a] = 0;
            lost = a;
        }
    }
    if (action >= 0 && action < count) {
        keys[action] = key;
    }
    return lost;
}

void wye_input_send_keyboard(struct wye_input *in)
{
    struct wye_keyboard k;
    uint8_t buf[WYE_KEYBOARD_LEN];
    wye_keystate_report(&in->remote, &k);
    in->out.send(in->out.ctx, WYE_MSG_KEYBOARD, buf, wye_keyboard_pack(&k, buf));
}

static bool hotkey(struct wye_input *in, const struct wye_input_event *ev)
{
    if (ev->down && !ev->repeat) {
        in->hotkey_held = true;
        in->chord_used = false;
    } else if (!ev->down) {
        if (in->hotkey_held && !in->chord_used) {
            in->out.action(in->out.ctx, WYE_ACT_TOGGLE);
        }
        in->hotkey_held = false;
    }
    return true; /* the hotkey never reaches either machine */
}

static bool chord(struct wye_input *in, const struct wye_input_event *ev)
{
    if (!in->hotkey_held || !ev->down) {
        return false;
    }
    for (int a = WYE_ACT_PASTE; a < WYE_ACT_COUNT; ++a) {
        if (in->chord_keys[a] && in->chord_keys[a] == ev->code) {
            in->chord_used = true;
            wye_usage_set(&in->chord_held, (uint8_t)ev->code, true);
            if (!ev->repeat) {
                in->out.action(in->out.ctx, (enum wye_input_action)a);
            }
            return true;
        }
    }
    return false;
}

bool wye_input_key(struct wye_input *in, const struct wye_input_event *ev)
{
    uint8_t usage = (uint8_t)ev->code;
    if (ev->code == in->hotkey) {
        return hotkey(in, ev);
    }
    if (chord(in, ev)) {
        return true;
    }
    if (!ev->down && wye_usage_has(&in->chord_held, usage)) {
        wye_usage_set(&in->chord_held, usage, false);
        return true;
    }
    if (!ev->down && wye_usage_has(&in->orphans, usage)) {
        wye_usage_set(&in->orphans, usage, false);
        if (in->mode == WYE_FWD_OFF) {
            return true;
        }
    }
    if (!ev->down && wye_usage_has(&in->local_keys, usage)) {
        wye_usage_set(&in->local_keys, usage, false);
        return false; /* pressed locally before forwarding began */
    }
    if (in->mode == WYE_FWD_OFF) {
        if (ev->down) {
            wye_usage_set(&in->local_keys, usage, true);
        }
        return false;
    }
    if (ev->repeat) {
        return true; /* the target generates its own auto-repeat */
    }
    bool changed = ev->down ? wye_keystate_press(&in->remote, usage)
                            : wye_keystate_release(&in->remote, usage);
    if (changed) {
        wye_input_send_keyboard(in);
    }
    return true;
}
