/*
 * Input routing: decides per event whether it goes to the target or stays
 * local, handles the hotkey and its chords, and replicates keyboard/mouse
 * state to the target. Keys pressed before a mode switch are tracked so
 * their releases reach the side that saw the press: nothing ever sticks.
 *
 * Not thread-safe; the engine serialises calls.
 */
#ifndef WYE_INPUT_H
#define WYE_INPUT_H

#include "input_event.h"
#include "keystate.h"

#include <stddef.h>

enum wye_fwd_mode {
    WYE_FWD_OFF = 0,
    WYE_FWD_RELATIVE,   /* full capture: everything goes to the target */
};

enum wye_input_action {
    WYE_ACT_TOGGLE = 1,    /* hotkey tapped */
    WYE_ACT_PASTE,         /* hotkey + V */
    WYE_ACT_JIGGLER,       /* hotkey + J */
    WYE_ACT_CTRL_ALT_DEL,  /* hotkey + Delete */
    WYE_ACT_SHOW_WINDOW,   /* hotkey + O */
    WYE_ACT_RELEASE,       /* hotkey + Esc: panic, back to local */
    WYE_ACT_MINI,          /* hotkey + M: mini panel <-> full window */
    WYE_ACT_FOLDER,        /* hotkey + F: open the shared folder */
    WYE_ACT_COUNT,
};

struct wye_input_out {
    void *ctx;
    void (*send)(void *ctx, uint8_t type, const void *payload, size_t len);
    void (*action)(void *ctx, enum wye_input_action action);
    void (*mode_changed)(void *ctx, enum wye_fwd_mode mode);
};

struct wye_input {
    struct wye_input_out out;
    enum wye_fwd_mode mode;
    uint16_t hotkey;
    bool hotkey_held, chord_used;
    uint8_t chord_keys[WYE_ACT_COUNT];   /* HID usage per action (0 = no shortcut); works in both modes */
    struct wye_usage_set local_keys;   /* pressed while local: releases pass through */
    struct wye_usage_set orphans;      /* held on the target when forwarding ended */
    struct wye_usage_set chord_held;   /* pressed as a shortcut: their releases go nowhere */
    struct wye_keystate remote;
    uint8_t local_buttons, remote_buttons, orphan_buttons;
    int sensitivity;                   /* percent */
    bool invert_wheel;
    int rem_x, rem_y, wheel_acc, pan_acc;
};

void wye_input_init(struct wye_input *in, const struct wye_input_out *out);
void wye_input_default_chords(uint8_t keys[WYE_ACT_COUNT]);
bool wye_input_event(struct wye_input *in, const struct wye_input_event *ev);
void wye_input_set_mode(struct wye_input *in, enum wye_fwd_mode mode);

/* Internal (input_keys.c / input_pointer.c). */
bool wye_input_key(struct wye_input *in, const struct wye_input_event *ev);
bool wye_input_pointer(struct wye_input *in, const struct wye_input_event *ev);
void wye_input_send_keyboard(struct wye_input *in);
void wye_input_send_pointer(struct wye_input *in, int dx, int dy, int wheel, int pan);

#endif
