/* Host link: background port opens and failed or short writes. */
#include "sim_transport.h"
#include "test.h"

static struct sim_transport g_t;
static struct wye_link g_link;

static void setup(void)
{
    sim_transport_init(&g_t);
    wye_link_init(&g_link, &g_t.ops, NULL);
}

static bool opening;
static void *pending_open(void *ctx, const char *path, char *err, size_t len)
{
    (void)path; (void)err; (void)len;
    if (!opening) ((struct sim_transport *)ctx)->dev.host_present = true;
    return opening ? NULL : ctx;
}
static bool open_pending(void *ctx) { (void)ctx; return opening; }

TEST(link_waits_for_background_open_before_starting_handshake)
{
    setup();
    g_t.ops.open = pending_open;
    g_t.ops.open_pending = open_pending;
    opening = true;
    sim_transport_run(&g_t, &g_link, 6000); /* longer than handshake + blacklist time */
    CHECK_EQ(g_link.info.state, WYE_LINK_SEARCHING);
    CHECK(g_link.port == NULL);
    opening = false;
    sim_transport_run(&g_t, &g_link, 1000);
    CHECK_EQ(g_link.info.state, WYE_LINK_READY);
}

static int failed_write(void *ctx, void *port, const uint8_t *data, size_t len)
{
    (void)ctx; (void)port; (void)data; (void)len;
    return -1;
}

TEST(link_retires_failed_writes_after_outstanding_read)
{
    setup();
    sim_transport_run(&g_t, &g_link, 1000);
    void *port = g_link.port;
    g_t.ops.write = failed_write;
    CHECK(!wye_link_send(&g_link, WYE_NODE_TARGET, WYE_MSG_RELEASE_ALL, NULL, 0));
    CHECK(g_link.port == port); /* UI/capture writer must not close a concurrent read */
    wye_link_process(&g_link, NULL, 0);
    CHECK(g_link.port == NULL);
    CHECK_EQ(g_link.info.state, WYE_LINK_SEARCHING);
}

static int short_write(void *ctx, void *port, const uint8_t *data, size_t len)
{
    (void)ctx; (void)port; (void)data;
    return len > 1 ? (int)len / 2 : (int)len; /* the Pico is busy erasing flash */
}

TEST(link_survives_short_writes)
{
    setup();
    sim_transport_run(&g_t, &g_link, 1000);
    void *port = g_link.port;
    g_t.ops.write = short_write;
    CHECK(!wye_link_send(&g_link, WYE_NODE_TARGET, WYE_MSG_RELEASE_ALL, NULL, 0));
    wye_link_process(&g_link, NULL, 0);
    CHECK(g_link.port == port);
}

static uint32_t g_boot;  /* the simulated target restarts: its uptime counts from here */
static uint32_t rebooted_now(void *ctx) { return ((struct sim_device *)ctx)->now - g_boot; }
static int g_left;
static void count_left(void *ctx, enum wye_link_state from, enum wye_link_state to)
{
    (void)ctx;
    g_left += from == WYE_LINK_READY && to == WYE_LINK_BRIDGE;
}

TEST(link_notices_a_fast_target_restart)
{
    sim_transport_init(&g_t);
    struct wye_link_listener li = {NULL, NULL, count_left};
    wye_link_init(&g_link, &g_t.ops, &li);
    g_left = 0;
    sim_transport_run(&g_t, &g_link, 3000);
    CHECK_EQ(g_link.info.state, WYE_LINK_READY);
    g_boot = g_t.dev.now;
    g_t.dev.target_hooks.now_ms = rebooted_now; /* back within one ping: no timeout fires */
    sim_transport_run(&g_t, &g_link, 2000);
    CHECK_EQ(g_left, 1);
    CHECK_EQ(g_link.info.state, WYE_LINK_READY);
}
