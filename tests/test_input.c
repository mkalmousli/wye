/* Input routing invariants: no stuck keys across mode switches, chords. */
#include "hid_usage.h"
#include "wye_proto.h"
#include "input.h"
#include "test.h"

static struct {
    struct wye_input in;
    struct wye_keyboard last_kbd;
    struct wye_mouse last_mouse;
    int sends, release_alls, toggles, pastes;
} g;

static void send(void *ctx, uint8_t type, const void *p, size_t len)
{
    (void)ctx;
    g.sends++;
    if (type == WYE_MSG_KEYBOARD) {
        wye_keyboard_unpack(p, len, &g.last_kbd);
    } else if (type == WYE_MSG_MOUSE) {
        wye_mouse_unpack(p, len, &g.last_mouse);
    } else if (type == WYE_MSG_RELEASE_ALL) {
        g.release_alls++;
    }
}

static void action(void *ctx, enum wye_input_action a)
{
    (void)ctx;
    g.toggles += a == WYE_ACT_TOGGLE;
    g.pastes += a == WYE_ACT_PASTE;
}

static void setup(void)
{
    memset(&g, 0, sizeof(g));
    struct wye_input_out out = {NULL, send, action, NULL};
    wye_input_init(&g.in, &out);
}

static bool key(uint16_t usage, bool down)
{
    struct wye_input_event ev = {WYE_EV_KEY, down, false, usage, 0, 0};
    return wye_input_event(&g.in, &ev);
}

TEST(input_local_key_released_after_forwarding_starts)
{
    setup();
    CHECK(!key(HID_KEY_LEFT_SHIFT, true));        /* local */
    wye_input_set_mode(&g.in, WYE_FWD_RELATIVE);
    CHECK(!key(HID_KEY_LEFT_SHIFT, false));       /* release still goes local */
    CHECK_EQ(g.sends, 0);                         /* target never saw shift */
}

TEST(input_remote_key_release_swallowed_after_return)
{
    setup();
    wye_input_set_mode(&g.in, WYE_FWD_RELATIVE);
    CHECK(key(HID_KEY_A, true));
    CHECK_EQ(g.last_kbd.keys[0], HID_KEY_A);
    wye_input_set_mode(&g.in, WYE_FWD_OFF);
    CHECK_EQ(g.release_alls, 1);
    CHECK(key(HID_KEY_A, false));                 /* stray release must not reach local OS */
}

TEST(input_hotkey_tap_toggles_and_chord_does_not)
{
    setup();
    CHECK(key(HID_KEY_F12, true));
    CHECK(key(HID_KEY_F12, false));
    CHECK_EQ(g.toggles, 1);
    CHECK(key(HID_KEY_F12, true));
    CHECK(key(HID_KEY_V, true));
    CHECK(key(HID_KEY_F12, false));
    CHECK_EQ(g.toggles, 1);
    CHECK_EQ(g.pastes, 1);
}

TEST(input_rollover_reports_latest_six)
{
    setup();
    wye_input_set_mode(&g.in, WYE_FWD_RELATIVE);
    for (uint16_t k = HID_KEY_A; k < HID_KEY_A + 8; ++k) {
        key(k, true);
    }
    CHECK_EQ(g.last_kbd.keys[0], HID_KEY_A + 2);
    CHECK_EQ(g.last_kbd.keys[5], HID_KEY_A + 7);
}

TEST(input_wheel_accumulates_hi_res)
{
    setup();
    wye_input_set_mode(&g.in, WYE_FWD_RELATIVE);
    struct wye_input_event ev = {WYE_EV_WHEEL, false, false, 0, 0, 60};
    wye_input_event(&g.in, &ev);
    CHECK_EQ(g.sends, 0);
    wye_input_event(&g.in, &ev);
    CHECK_EQ(g.last_mouse.wheel, 1);
}

TEST(input_sensitivity_keeps_remainder)
{
    setup();
    g.in.sensitivity = 50;
    wye_input_set_mode(&g.in, WYE_FWD_RELATIVE);
    struct wye_input_event ev = {WYE_EV_MOVE, false, false, 0, 1, 0};
    for (int i = 0; i < 4; ++i) {
        wye_input_event(&g.in, &ev);
    }
    CHECK_EQ(g.sends, 2);   /* 4 counts at 50% = 2 output counts */
}
