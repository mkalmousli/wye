/* Firmware update protocol, end to end through the bridge relay. */
#include "sim.h"
#include "test.h"

#include <stdlib.h>

static struct sim_device g_sim;
static uint8_t g_image[10000];

/* Reads host-bound traffic and returns the last FW_ACK status (or -1). */
static int last_ack(struct sim_device *s, uint32_t *next)
{
    uint8_t buf[4096];
    struct wye_decoder d;
    struct wye_packet p;
    int status = -1;
    wye_decoder_init(&d);
    size_t n = sim_host_read(s, buf, sizeof(buf));
    for (size_t i = 0; i < n; ++i) {
        if (wye_decoder_feed(&d, buf[i], &p) == WYE_DECODE_PACKET && p.type == WYE_MSG_FW_ACK) {
            status = p.payload[0];
            *next = wye_get_u32(p.payload + 1);
        }
    }
    return status;
}

static void send_chunk(struct sim_device *s, uint32_t off)
{
    uint8_t p[4 + WYE_FW_CHUNK];
    uint32_t len = sizeof(g_image) - off < WYE_FW_CHUNK ? (uint32_t)sizeof(g_image) - off : WYE_FW_CHUNK;
    wye_put_u32(p, off);
    memcpy(p + 4, g_image + off, len);
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_FW_DATA, p, 4 + len);
}

static void begin(struct sim_device *s, uint32_t crc)
{
    uint8_t p[8];
    wye_put_u32(p, sizeof(g_image));
    wye_put_u32(p + 4, crc);
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_FW_BEGIN, p, sizeof(p));
}

TEST(fwupdate_full_transfer_installs)
{
    struct sim_device *s = &g_sim;
    uint32_t next = 0;
    sim_init(s);
    for (size_t i = 0; i < sizeof(g_image); ++i) {
        g_image[i] = (uint8_t)rand();
    }
    begin(s, wye_crc32(g_image, sizeof(g_image)));
    CHECK_EQ(last_ack(s, &next), WYE_FW_OK);
    for (uint32_t off = 0; off < sizeof(g_image); off += WYE_FW_CHUNK) {
        send_chunk(s, off);
        CHECK_EQ(last_ack(s, &next), WYE_FW_OK);
        if (off == WYE_FW_CHUNK * 3) {
            send_chunk(s, off);    /* retransmit must be harmless */
            CHECK_EQ(last_ack(s, &next), WYE_FW_OK);
        }
    }
    CHECK_EQ(next, sizeof(g_image));
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_FW_COMMIT, NULL, 0);
    CHECK_EQ(last_ack(s, &next), WYE_FW_OK);
    CHECK_MEM(s->target_flash.mem, g_image, sizeof(g_image));
    CHECK_EQ(s->target_flash.installed_size, 0u);  /* deferred until ack drains */
    sim_advance(s, WYE_NODE_ACT_DELAY_MS + 1);
    CHECK_EQ(s->target_flash.installed_size, sizeof(g_image));
}

TEST(fwupdate_rejects_bad_crc_and_gaps)
{
    struct sim_device *s = &g_sim;
    uint32_t next = 0;
    sim_init(s);
    begin(s, 0xdeadbeef);
    last_ack(s, &next);
    send_chunk(s, WYE_FW_CHUNK);   /* gap */
    CHECK_EQ(last_ack(s, &next), WYE_FW_ERR_OFFSET);
    CHECK_EQ(next, 0u);
    for (uint32_t off = 0; off < sizeof(g_image); off += WYE_FW_CHUNK) {
        send_chunk(s, off);
    }
    last_ack(s, &next);
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_FW_COMMIT, NULL, 0);
    CHECK_EQ(last_ack(s, &next), WYE_FW_ERR_CRC);
    sim_advance(s, 200);
    CHECK_EQ(s->target_flash.installed_size, 0u);
}
