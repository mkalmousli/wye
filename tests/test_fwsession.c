/* Host-driven firmware updates end to end: app session -> link -> bridge (-> wires -> target). */
#include "fw/fw_session.h"
#include "sim_transport.h"
#include "test.h"

#include <stdlib.h>
#include <string.h>

#define BLOCKS 24 /* 24 x 256 bytes = a 6 KiB image */

static struct sim_transport g_t;
static struct wye_link g_link;
static struct wye_fw_session g_fw;
static uint8_t g_uf2[512 * BLOCKS];
static uint8_t g_bin[256 * BLOCKS];

static void on_packet(void *ctx, const struct wye_packet *pkt)
{
    (void)ctx;
    wye_fw_session_packet(&g_fw, pkt, g_t.dev.now);
}

/* A UF2 for version 3.1.0 (the simulated devices run 3.0.0). */
static const struct wye_fw_image *make_image(uint8_t role)
{
    static struct wye_fw_image img;
    memset(g_uf2, 0, sizeof(g_uf2));
    for (uint32_t b = 0; b < BLOCKS; ++b) {
        uint8_t *blk = g_uf2 + 512 * b;
        wye_put_u32(blk, 0x0a324655u);
        wye_put_u32(blk + 4, 0x9e5d5157u);
        wye_put_u32(blk + 8, 0x2000);
        wye_put_u32(blk + 12, WYE_FLASH_BASE + 256u * b);
        wye_put_u32(blk + 16, 256);
        wye_put_u32(blk + 28, WYE_UF2_FAMILY_RP2040);
        for (int i = 0; i < 256; ++i) {
            blk[32 + i] = g_bin[256 * b + (uint32_t)i] = (uint8_t)rand();
        }
    }
    img = (struct wye_fw_image){role, role == WYE_ROLE_TARGET ? "target" : "bridge", g_uf2, sizeof(g_uf2), 3, 1, 0};
    return &img;
}

static void run(uint32_t ms, enum wye_fw_phase stop_at)
{
    for (uint32_t i = 0; i < ms && g_fw.phase != stop_at && g_fw.phase != WYE_FWS_FAILED; ++i) {
        sim_transport_run(&g_t, &g_link, 1);
        wye_fw_session_tick(&g_fw, &g_link, g_t.dev.now);
    }
}

static void setup(void)
{
    struct wye_link_listener listener = {NULL, on_packet, NULL};
    sim_transport_init(&g_t);
    wye_link_init(&g_link, &g_t.ops, &listener);
    wye_fw_session_reset(&g_fw);
    sim_transport_run(&g_t, &g_link, 1000);
}

TEST(fwsession_updates_target_through_bridge)
{
    setup();
    CHECK_EQ(g_link.info.state, WYE_LINK_READY);
    CHECK(wye_fw_session_start(&g_fw, WYE_NODE_TARGET, make_image(WYE_ROLE_TARGET), g_t.dev.now));
    run(20000, WYE_FWS_REBOOT);
    CHECK_EQ(g_fw.phase, WYE_FWS_REBOOT);
    CHECK_MEM(g_t.dev.target_flash.mem, g_bin, sizeof(g_bin));
    run(WYE_NODE_ACT_DELAY_MS + 5, WYE_FWS_IDLE);
    CHECK_EQ(g_t.dev.target_flash.installed_size, sizeof(g_bin));
    /* The target restarts: silent for a moment, then back running 3.1.0. */
    g_t.dev.cable_connected = false;
    run(WYE_LINK_TIMING_TARGET_TIMEOUT_MS + 100, WYE_FWS_DONE);
    g_t.dev.target.node.identity.fw_minor = 1;
    g_t.dev.cable_connected = true;
    run(3000, WYE_FWS_DONE);
    CHECK_EQ(g_fw.phase, WYE_FWS_DONE);
}

TEST(fwsession_updates_bridge_without_bootsel)
{
    setup();
    CHECK(wye_fw_session_start(&g_fw, WYE_NODE_BRIDGE, make_image(WYE_ROLE_BRIDGE), g_t.dev.now));
    run(20000, WYE_FWS_REBOOT);
    CHECK_EQ(g_fw.phase, WYE_FWS_REBOOT);
    CHECK_MEM(g_t.dev.bridge_flash.mem, g_bin, sizeof(g_bin));
    /* The bridge restarts: its USB port goes away and comes back running 3.1.0. */
    g_t.plugged = false;
    run(300, WYE_FWS_DONE);
    g_t.dev.bridge.node.identity.fw_minor = 1;
    g_t.plugged = true;
    run(5000, WYE_FWS_DONE);
    CHECK_EQ(g_fw.phase, WYE_FWS_DONE);
    sim_transport_run(&g_t, &g_link, 1000); /* the target answers again through the new bridge */
    CHECK_EQ(g_link.info.state, WYE_LINK_READY);
}
