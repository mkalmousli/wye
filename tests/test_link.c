/* Host link state machine against the simulated bridge + target. */
#include "sim_transport.h"
#include "test.h"

static struct sim_transport g_t;
static struct wye_link g_link;

static void setup(void)
{
    sim_transport_init(&g_t);
    wye_link_init(&g_link, &g_t.ops, NULL);
}

TEST(link_connects_to_ready)
{
    setup();
    sim_transport_run(&g_t, &g_link, 1000);
    CHECK_EQ(g_link.info.state, WYE_LINK_READY);
    CHECK_EQ(g_link.info.bridge.hello.role, WYE_ROLE_BRIDGE);
    CHECK_EQ(g_link.info.target.hello.role, WYE_ROLE_TARGET);
    CHECK(g_link.info.target_state & WYE_TSTATE_USB_MOUNTED);
}

TEST(link_cable_pull_and_restore)
{
    setup();
    sim_transport_run(&g_t, &g_link, 1000);
    g_t.dev.cable_connected = false;
    sim_transport_run(&g_t, &g_link, WYE_LINK_TIMING_TARGET_TIMEOUT_MS + 150);
    CHECK_EQ(g_link.info.state, WYE_LINK_BRIDGE);
    sim_transport_run(&g_t, &g_link, 3000);
    CHECK_EQ(g_link.info.state, WYE_LINK_BRIDGE); /* bridge must stay up */
    g_t.dev.cable_connected = true;
    sim_transport_run(&g_t, &g_link, 500);
    CHECK_EQ(g_link.info.state, WYE_LINK_READY);
}

TEST(link_usb_unplug_and_replug)
{
    setup();
    sim_transport_run(&g_t, &g_link, 1000);
    g_t.plugged = false;
    sim_transport_run(&g_t, &g_link, 50);
    CHECK_EQ(g_link.info.state, WYE_LINK_SEARCHING);
    CHECK(g_link.port == NULL);
    g_t.plugged = true;
    sim_transport_run(&g_t, &g_link, 1500);
    CHECK_EQ(g_link.info.state, WYE_LINK_READY);
}

TEST(link_hung_bridge_is_dropped)
{
    setup();
    sim_transport_run(&g_t, &g_link, 1000);
    g_t.silent = true;
    sim_transport_run(&g_t, &g_link, WYE_LINK_TIMING_BRIDGE_TIMEOUT_MS + 100);
    CHECK(g_link.info.state != WYE_LINK_READY);
    CHECK(g_link.info.state != WYE_LINK_BRIDGE);
    g_t.silent = false;
    sim_transport_run(&g_t, &g_link, 3000);
    CHECK_EQ(g_link.info.state, WYE_LINK_READY);
}

TEST(link_legacy_device_detected_and_not_spammed)
{
    setup();
    g_t.legacy_ids = true;
    g_t.silent = true;
    sim_transport_run(&g_t, &g_link, 2000);
    CHECK_EQ(g_link.info.state, WYE_LINK_LEGACY);
    CHECK(strcmp(g_link.info.legacy_port, "SIM0") == 0);
    int opens = g_t.opens;
    sim_transport_run(&g_t, &g_link, 20000);
    CHECK_EQ(g_t.opens, opens);   /* not re-probed every scan */
    g_t.plugged = false;
    sim_transport_run(&g_t, &g_link, 1000);
    CHECK_EQ(g_link.info.state, WYE_LINK_SEARCHING);
}

TEST(link_measures_rtt)
{
    setup();
    sim_transport_run(&g_t, &g_link, 1500);
    CHECK(g_link.info.target.rtt_ms <= 2);
    CHECK(g_link.info.target.pong.token != 0);
}
