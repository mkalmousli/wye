/* Shared drive, target side, through the real bridge relay in the simulator. */
#include "sim.h"
#include "test.h"

static struct sim_device g_sim;

/* Collects host-bound packets of one type; returns how many arrived. */
static int take(struct sim_device *s, uint8_t type, struct wye_packet *last)
{
    uint8_t buf[8192];
    struct wye_decoder d;
    struct wye_packet p;
    int count = 0;
    wye_decoder_init(&d);
    size_t n = sim_host_read(s, buf, sizeof(buf));
    for (size_t i = 0; i < n; ++i) {
        if (wye_decoder_feed(&d, buf[i], &p) == WYE_DECODE_PACKET && p.type == type) {
            *last = p;
            count++;
        }
    }
    return count;
}

static void insert(struct sim_device *s, uint32_t blocks)
{
    uint8_t p[5] = {1};
    wye_put_u32(p + 1, blocks);
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_DISK_SET, p, sizeof(p));
}

static void serve_block(struct sim_device *s, uint32_t lba, uint8_t fill)
{
    uint8_t p[6 + WYE_DISK_CHUNK];
    for (uint16_t off = 0; off < WYE_DISK_BLOCK; off += WYE_DISK_CHUNK) {
        uint16_t n = WYE_DISK_BLOCK - off < WYE_DISK_CHUNK ? (uint16_t)(WYE_DISK_BLOCK - off) : WYE_DISK_CHUNK;
        wye_put_u32(p, lba);
        wye_put_u16(p + 4, off);
        memset(p + 6, fill, n);
        sim_send(s, WYE_NODE_TARGET, WYE_MSG_DISK_DATA, p, 6u + n);
    }
}

TEST(disk_reads_ahead_from_host)
{
    struct sim_device *s = &g_sim;
    struct wye_packet pkt;
    uint8_t block[WYE_DISK_BLOCK];
    sim_init(s);
    CHECK_EQ(wye_disk_read(&s->target, 0, block), -1); /* no medium yet */
    insert(s, 100);
    CHECK_EQ(take(s, WYE_MSG_DISK_STATUS, &pkt), 1);
    CHECK_EQ(pkt.payload[0], WYE_DISK_PRESENT);
    CHECK_EQ(wye_disk_read(&s->target, 10, block), 0);
    CHECK_EQ(take(s, WYE_MSG_DISK_READ, &pkt), 1);
    CHECK_EQ(wye_get_u32(pkt.payload), 10u);
    CHECK_EQ(pkt.payload[4], WYE_DISK_READAHEAD);
    for (uint32_t i = 0; i < WYE_DISK_READAHEAD; ++i) {
        serve_block(s, 10 + i, (uint8_t)(0x40 + i));
    }
    CHECK_EQ(wye_disk_read(&s->target, 10, block), (int32_t)WYE_DISK_BLOCK);
    CHECK_EQ(block[511], 0x40);
    CHECK_EQ(wye_disk_read(&s->target, 11, block), (int32_t)WYE_DISK_BLOCK); /* from the read-ahead */
    CHECK_EQ(block[0], 0x41);
    CHECK(take(s, WYE_MSG_DISK_READ, &pkt) == 0 && wye_disk_read(&s->target, 100, block) == -1); /* past the end */
}

TEST(disk_write_waits_for_host_ack)
{
    struct sim_device *s = &g_sim;
    struct wye_packet pkt;
    uint8_t block[WYE_DISK_BLOCK], ack[5] = {0};
    sim_init(s);
    insert(s, 100);
    memset(block, 0x5a, sizeof(block));
    CHECK_EQ(wye_disk_write(&s->target, 7, block), 0);
    CHECK_EQ(take(s, WYE_MSG_DISK_WRITE, &pkt), 3); /* 512 bytes in three chunks */
    CHECK_EQ(wye_get_u16(pkt.payload + 4), 448u);
    CHECK_EQ(wye_disk_write(&s->target, 7, block), 0); /* still waiting */
    wye_put_u32(ack, 7);
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_DISK_WACK, ack, sizeof(ack));
    CHECK_EQ(wye_disk_write(&s->target, 7, block), (int32_t)WYE_DISK_BLOCK);
    CHECK(wye_disk_flags(&s->target) & WYE_DISK_BUSY);
    sim_advance(s, WYE_DISK_IDLE_MS + 10);
    CHECK(!(wye_disk_flags(&s->target) & WYE_DISK_BUSY));
    wye_disk_eject(&s->target);
    CHECK(wye_disk_flags(&s->target) & WYE_DISK_EJECTED);
    CHECK_EQ(wye_disk_write(&s->target, 7, block), -1);
}

TEST(disk_gives_up_when_host_is_silent)
{
    struct sim_device *s = &g_sim;
    uint8_t block[WYE_DISK_BLOCK];
    sim_init(s);
    insert(s, 100);
    CHECK_EQ(wye_disk_read(&s->target, 0, block), 0);
    sim_advance(s, WYE_DISK_TIMEOUT_MS + 10);
    CHECK_EQ(wye_disk_read(&s->target, 0, block), -1);
}
