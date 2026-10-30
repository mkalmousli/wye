/* Target firmware logic, driven through the bridge like real traffic. */
#include "sim.h"
#include "test.h"

static struct sim_device g_sim;

static void send_kbd(struct sim_device *s, uint8_t mods, uint8_t key)
{
    struct wye_keyboard k = {mods, {key, 0, 0, 0, 0, 0}};
    uint8_t buf[WYE_KEYBOARD_LEN];
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_KEYBOARD, buf, wye_keyboard_pack(&k, buf));
}

static void send_mouse(struct sim_device *s, uint8_t buttons, int16_t dx, int16_t dy)
{
    struct wye_mouse m = {buttons, dx, dy, 0, 0};
    uint8_t buf[WYE_MOUSE_LEN];
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_MOUSE, buf, wye_mouse_pack(&m, buf));
}

TEST(target_fast_press_release_not_lost)
{
    struct sim_device *s = &g_sim;
    sim_init(s);
    s->hid_ready = false;          /* PC has not polled yet */
    send_kbd(s, 0, 0x04);
    send_kbd(s, 0, 0);
    s->hid_ready = true;
    sim_advance(s, 5);
    CHECK_EQ(s->report_count, 2);
    CHECK_EQ(s->reports[0].kbd.keys[0], 0x04);
    CHECK_EQ(s->reports[1].kbd.keys[0], 0);
}

TEST(target_motion_coalesces_but_clicks_do_not)
{
    struct sim_device *s = &g_sim;
    sim_init(s);
    s->hid_ready = false;
    send_mouse(s, 0, 5, 5);
    send_mouse(s, 0, 5, 5);
    send_mouse(s, WYE_BTN_LEFT, 0, 0);
    send_mouse(s, 0, 0, 0);
    s->hid_ready = true;
    sim_advance(s, 5);
    CHECK_EQ(s->report_count, 3);
    CHECK_EQ(s->reports[0].dx, 10);
    CHECK_EQ(s->reports[1].buttons, WYE_BTN_LEFT);
    CHECK_EQ(s->reports[2].buttons, 0);
}

TEST(target_boot_protocol_splits_large_motion)
{
    struct sim_device *s = &g_sim;
    sim_init(s);
    s->rel_max = 127;
    send_mouse(s, 0, 300, -20);
    sim_advance(s, 5);
    CHECK_EQ(s->report_count, 3);
    CHECK_EQ(s->reports[0].dx + s->reports[1].dx + s->reports[2].dx, 300);
    CHECK_EQ(s->reports[0].dy, -20);
}

TEST(target_failsafe_releases_when_cable_pulled)
{
    struct sim_device *s = &g_sim;
    sim_init(s);
    send_kbd(s, 0x02, 0x04);       /* shift + a held */
    sim_advance(s, 10);
    s->cable_connected = false;
    sim_advance(s, WYE_TARGET_FAILSAFE_MS + 10);
    struct sim_report *last = &s->reports[s->report_count - 1];
    CHECK_EQ(last->kind, 'k');
    CHECK_EQ(last->kbd.modifiers, 0);
    CHECK_EQ(last->kbd.keys[0], 0);
}

TEST(bridge_releases_target_when_host_closes)
{
    struct sim_device *s = &g_sim;
    sim_init(s);
    send_mouse(s, WYE_BTN_LEFT, 0, 0);
    sim_advance(s, 5);
    s->host_present = false;
    sim_advance(s, 5);              /* well before the target failsafe */
    CHECK_EQ(s->reports[s->report_count - 1].buttons, 0);
}

TEST(target_remote_wakeup_when_suspended)
{
    struct sim_device *s = &g_sim;
    sim_init(s);
    s->suspended = true;
    send_kbd(s, 0, 0x2c);
    sim_advance(s, 2);
    CHECK_EQ(s->wakeups, 1);
}
