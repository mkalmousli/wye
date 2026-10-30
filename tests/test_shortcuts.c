/* Hotkey shortcuts: configurable, and working whether or not the target is being controlled. */
#include "hid_usage.h"
#include "input/input.h"
#include "test.h"

static int g_action;

static void send(void *ctx, uint8_t type, const void *payload, size_t len)
{
    (void)ctx, (void)type, (void)payload, (void)len;
}

static void action(void *ctx, enum wye_input_action a)
{
    (void)ctx;
    g_action = a;
}

static void key(struct wye_input *in, uint16_t code, bool down)
{
    struct wye_input_event ev = {.type = WYE_EV_KEY, .down = down, .code = code};
    CHECK(wye_input_event(in, &ev)); /* chords never reach either computer */
}

static void chord(struct wye_input *in, uint16_t code)
{
    key(in, HID_KEY_F12, true);
    key(in, code, true);
    key(in, code, false);
    key(in, HID_KEY_F12, false);
}

TEST(shortcuts_work_locally_and_while_controlling)
{
    struct wye_input in;
    struct wye_input_out out = {NULL, send, action, NULL};
    wye_input_init(&in, &out);
    chord(&in, HID_KEY_J);
    CHECK_EQ(g_action, WYE_ACT_JIGGLER);               /* local mode */
    in.chord_keys[WYE_ACT_JIGGLER] = HID_KEY_A + 6;     /* remapped to G */
    wye_input_set_mode(&in, WYE_FWD_RELATIVE);
    g_action = 0;
    chord(&in, HID_KEY_A + 6);
    CHECK_EQ(g_action, WYE_ACT_JIGGLER);               /* while controlling */
    in.chord_keys[WYE_ACT_MINI] = 0;                   /* switched off */
    g_action = 0;
    key(&in, HID_KEY_F12, true);
    struct wye_input_event m = {.type = WYE_EV_KEY, .down = true, .code = HID_KEY_M};
    wye_input_event(&in, &m);
    CHECK(g_action != WYE_ACT_MINI);
}

TEST(learning_a_shortcut_passes_over_the_hotkey_and_modifiers)
{
    CHECK(!wye_shortcut_key_accepted(HID_KEY_F12, HID_KEY_F12)); /* holding it first, as "F12 + J" suggests */
    CHECK(!wye_shortcut_key_accepted(HID_KEY_LEFT_CTRL, HID_KEY_F12));
    CHECK(!wye_shortcut_key_accepted(HID_KEY_RIGHT_GUI, HID_KEY_F12));
    CHECK(wye_shortcut_key_accepted(HID_KEY_J, HID_KEY_F12));
}

TEST(a_key_only_does_one_thing)
{
    uint8_t keys[WYE_ACT_COUNT];
    wye_input_default_chords(keys);
    CHECK_EQ(wye_shortcut_assign(keys, WYE_ACT_COUNT, WYE_ACT_MINI, HID_KEY_J), WYE_ACT_JIGGLER);
    CHECK_EQ(keys[WYE_ACT_MINI], HID_KEY_J);
    CHECK_EQ(keys[WYE_ACT_JIGGLER], 0);
    CHECK_EQ(wye_shortcut_assign(keys, WYE_ACT_COUNT, WYE_ACT_MINI, HID_KEY_J), 0); /* the same again: nothing lost */
    /* Choosing a new hotkey takes the key from a shortcut too. */
    CHECK_EQ(wye_shortcut_assign(keys, WYE_ACT_COUNT, -1, HID_KEY_V), WYE_ACT_PASTE);
    CHECK_EQ(keys[WYE_ACT_PASTE], 0);
}
