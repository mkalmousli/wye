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

/* ----- editing routines ----- */
static struct wye_routine_event ev_mouse(uint32_t t, uint8_t buttons, int dx, int dy)
{
    struct wye_routine_event e = {.t = t, .type = WYE_MSG_MOUSE};
    struct wye_mouse m = {buttons, (int16_t)dx, (int16_t)dy, 0, 0};
    e.len = (uint8_t)wye_mouse_pack(&m, e.payload);
    return e;
}

static struct wye_routine_event ev_keys(uint32_t t, uint8_t key)
{
    struct wye_routine_event e = {.t = t, .type = WYE_MSG_KEYBOARD};
    struct wye_keyboard k = {0, {key, 0, 0, 0, 0, 0}};
    e.len = (uint8_t)wye_keyboard_pack(&k, e.payload);
    return e;
}

static void doc_fill(struct wye_routine_doc *d, const struct wye_routine_event *ev, size_t n)
{
    wye_routine_doc_free(d);
    d->ev = calloc(n + 8, sizeof(*d->ev));
    d->cap = n + 8;
    d->count = n;
    memcpy(d->ev, ev, n * sizeof(*ev));
}

TEST(routine_edit_delete_duplicate_move_delay_speed)
{
    struct wye_routine_doc d = {0};
    const struct wye_routine_event src[] = {ev_keys(100, 4), ev_keys(200, 0), ev_keys(500, 5), ev_keys(700, 0)};
    doc_fill(&d, src, 4);
    CHECK_EQ(wye_routine_doc_delay(&d, 0), 100);
    CHECK_EQ(wye_routine_doc_delay(&d, 2), 300);
    wye_routine_doc_set_delay(&d, 2, 50);
    CHECK(d.ev[2].t == 250 && d.ev[3].t == 450); /* what follows moves with it */
    wye_routine_doc_delete(&d, 1, 2);
    CHECK_EQ(d.count, 2);
    CHECK_EQ(d.ev[1].t, 200);                    /* no empty wait is left behind */
    CHECK(wye_routine_doc_duplicate(&d, 0, 1));
    CHECK_EQ(d.count, 4);
    CHECK(d.ev[2].t > d.ev[1].t && d.ev[3].t > d.ev[2].t);
    CHECK_EQ(d.ev[2].payload[1], d.ev[0].payload[1]);
    uint8_t first = d.ev[0].payload[1];
    uint32_t t0 = d.ev[0].t;
    CHECK(wye_routine_doc_move(&d, 0, 1));
    CHECK(d.ev[0].t == t0 && d.ev[1].payload[1] == first); /* the event moved, the clock did not */
    CHECK(!wye_routine_doc_move(&d, 0, -1));
    CHECK(!wye_routine_doc_move(&d, 3, 1));
    t0 = d.ev[3].t;
    wye_routine_doc_speed(&d, 200);
    CHECK_EQ(d.ev[3].t, t0 / 2);
    wye_routine_doc_free(&d);
}

TEST(routine_compress_never_drops_what_matters)
{
    struct wye_routine_doc d = {0};
    const struct wye_routine_event src[] = {
        ev_keys(5000, 4), ev_keys(5001, 4),         /* a repeat of the same state: useless */
        ev_mouse(5010, 0, 1, 1), ev_mouse(5015, 0, 2, 2), ev_mouse(5020, 0, 3, 3),
        ev_mouse(5022, 1, 0, 0),                    /* a button change: must stay */
        ev_mouse(5024, 1, 0, 0),                    /* the same again: useless */
        ev_mouse(5026, 1, 4, 4), ev_keys(9000, 0),  /* then a long wait */
    };
    doc_fill(&d, src, 9);
    struct wye_routine_compress plain = {0};
    CHECK_EQ(wye_routine_doc_compress(&d, &plain), 2);
    CHECK_EQ(d.ev[0].t, 5000); /* nothing but the useless events changed */
    doc_fill(&d, src, 9);
    struct wye_routine_compress all = {true, WYE_ROUTINE_MERGE_MS, true, WYE_ROUTINE_MAX_PAUSE_MS};
    size_t removed = wye_routine_doc_compress(&d, &all);
    CHECK_EQ(removed, 4);
    CHECK_EQ(d.count, 5);
    CHECK_EQ(d.ev[0].t, 0);    /* no wait before the first event */
    struct wye_mouse m;
    CHECK(wye_mouse_unpack(d.ev[1].payload, d.ev[1].len, &m));
    CHECK(m.dx == 6 && m.dy == 6 && m.buttons == 0);
    CHECK(wye_mouse_unpack(d.ev[2].payload, d.ev[2].len, &m));
    CHECK(m.buttons == 1 && !m.dx);
    CHECK_EQ(d.ev[4].t - d.ev[3].t, WYE_ROUTINE_MAX_PAUSE_MS);
    wye_routine_doc_free(&d);
}

TEST(routine_doc_saves_and_loads)
{
    struct wye_routine_doc d = {0}, back = {0};
    const struct wye_routine_event src[] = {ev_keys(1, 4), ev_mouse(2, 0, -3, 7)};
    char err[64];
    doc_fill(&d, src, 2);
    CHECK(wye_routine_doc_save(&d, "out/test-routine.hwr"));
    CHECK(wye_routine_doc_load(&back, "out/test-routine.hwr", err, sizeof(err)));
    CHECK_EQ(back.count, 2);
    CHECK(back.ev[1].t == 2 && back.ev[1].type == WYE_MSG_MOUSE && memcmp(back.ev[1].payload, d.ev[1].payload, d.ev[1].len) == 0);
    remove("out/test-routine.hwr");
    wye_routine_doc_free(&d);
    wye_routine_doc_free(&back);
}

TEST(recorder_keeps_the_newest_events_for_watching)
{
    static struct wye_recorder r;
    struct wye_routine_event out[8];
    memset(&r, 0, sizeof(r));
    r.file = fopen("out/test-tail.hwr", "w");
    CHECK(r.file != NULL);
    for (uint8_t i = 0; i < WYE_ROUTINE_TAIL + 5; ++i) {
        uint8_t p[2] = {i, 0};
        wye_recorder_note(&r, i, WYE_MSG_CONSUMER, p, 2);
    }
    wye_recorder_stop(&r);
    CHECK_EQ(wye_recorder_tail(&r, out, 8), 8);
    CHECK_EQ(out[7].payload[0], WYE_ROUTINE_TAIL + 4);
    CHECK_EQ(out[0].payload[0], WYE_ROUTINE_TAIL - 3);
    remove("out/test-tail.hwr");
}
