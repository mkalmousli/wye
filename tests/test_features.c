/* Jiggler, typing, routine parsing and UF2 conversion. */
#include "features/jiggler.h"
#include "features/routine.h"
#include "features/typing.h"
#include "fw/fw_image.h"
#include "wye_msg.h"
#include "wye_proto.h"
#include "test.h"

#include <stdlib.h>

static struct { int sends; long sum_x, sum_y; struct wye_keyboard last; } g;

static bool capture(void *ctx, uint8_t type, const void *p, size_t len)
{
    (void)ctx;
    struct wye_mouse m;
    g.sends++;
    if (type == WYE_MSG_MOUSE && wye_mouse_unpack(p, len, &m)) {
        g.sum_x += m.dx;
        g.sum_y += m.dy;
    } else if (type == WYE_MSG_KEYBOARD) {
        wye_keyboard_unpack(p, len, &g.last);
    }
    return true;
}

static const struct wye_sender SENDER = {NULL, capture};

TEST(jiggler_returns_to_origin)
{
    static struct wye_jiggler j;
    memset(&g, 0, sizeof(g));
    j.interval_ms = 5000;
    wye_jiggler_set(&j, true, 0);
    for (uint64_t t = 0; t < 9000; ++t) {
        wye_jiggler_tick(&j, t, &SENDER);
    }
    CHECK(g.sends > 50);
    CHECK_EQ(j.phase, WYE_JIG_IDLE);
    CHECK_EQ(g.sum_x, 0);
    CHECK_EQ(g.sum_y, 0);
}

TEST(typing_presses_and_releases)
{
    static struct wye_typing t;
    memset(&g, 0, sizeof(g));
    CHECK_EQ(wye_typing_text(&t, "Hi!\r\n\x01"), 4u); /* CRLF -> one Enter, \x01 skipped */
    for (uint64_t now = 0; now < 200; ++now) {
        wye_typing_tick(&t, now, &SENDER);
    }
    CHECK_EQ(g.sends, 8);
    CHECK_EQ(g.last.keys[0], 0);
    uint8_t mods, key;
    CHECK(wye_ascii_to_hid('?', &mods, &key) && mods == 2 && key == 0x38);
    CHECK(wye_ascii_to_hid('0', &mods, &key) && mods == 0 && key == 0x27);
    CHECK(wye_ascii_to_hid(')', &mods, &key) && mods == 2 && key == 0x27);
}

TEST(routine_line_parsing)
{
    struct wye_routine_event ev;
    CHECK(wye_routine_parse_line("1234 16 02040000000000", &ev));
    CHECK_EQ(ev.t, 1234u);
    CHECK_EQ(ev.type, 16);
    CHECK_EQ(ev.len, 7);
    CHECK_EQ(ev.payload[1], 4);
    CHECK(!wye_routine_parse_line("# comment", &ev));
    CHECK(!wye_routine_parse_line("12 16 0", &ev));
    CHECK(!wye_routine_parse_line("", &ev));
}

TEST(uf2_to_bin_assembles_blocks)
{
    static uint8_t uf2[512 * 2];
    memset(uf2, 0, sizeof(uf2));
    for (int b = 0; b < 2; ++b) {
        uint8_t *blk = uf2 + 512 * b;
        wye_put_u32(blk, 0x0a324655u);
        wye_put_u32(blk + 4, 0x9e5d5157u);
        wye_put_u32(blk + 8, 0x2000);
        wye_put_u32(blk + 12, WYE_FLASH_BASE + 256u * (uint32_t)b);
        wye_put_u32(blk + 16, 256);
        wye_put_u32(blk + 28, WYE_UF2_FAMILY_RP2040);
        memset(blk + 32, 0x11 * (b + 1), 256);
    }
    uint8_t *bin;
    size_t len;
    CHECK(wye_uf2_to_bin(uf2, sizeof(uf2), &bin, &len));
    CHECK_EQ(len, 512u);
    CHECK_EQ(bin[0], 0x11);
    CHECK_EQ(bin[300], 0x22);
    free(bin);
    wye_put_u32(uf2 + 28, 0x12345678); /* wrong chip family */
    CHECK(!wye_uf2_to_bin(uf2, sizeof(uf2), &bin, &len));
}
