#include "wye_codec.h"
#include "wye_msg.h"
#include "test.h"

#include <stdlib.h>

TEST(crc_known_vectors)
{
    const uint8_t s[] = "123456789";
    CHECK_EQ(wye_crc16(s, 9), 0x29b1);
    CHECK_EQ(wye_crc32(s, 9), 0xcbf43926u);
}

TEST(cobs_roundtrip_random)
{
    uint8_t in[600], enc[700], dec[700];
    srand(1);
    for (int iter = 0; iter < 2000; ++iter) {
        size_t len = (size_t)(rand() % (int)sizeof(in));
        for (size_t i = 0; i < len; ++i) {
            in[i] = (uint8_t)(rand() % 4 == 0 ? 0 : rand());
        }
        size_t n = wye_cobs_encode(in, len, enc, sizeof(enc));
        CHECK(n > 0);
        CHECK(memchr(enc, 0, n) == NULL);
        CHECK_EQ(wye_cobs_decode(enc, n, dec, sizeof(dec)), len);
        CHECK_MEM(in, dec, len);
    }
}

static int decode_stream(const uint8_t *buf, size_t len, struct wye_packet *last, uint32_t *errors)
{
    struct wye_decoder d;
    wye_decoder_init(&d);
    int packets = 0;
    for (size_t i = 0; i < len; ++i) {
        if (wye_decoder_feed(&d, buf[i], last) == WYE_DECODE_PACKET) {
            packets++;
        }
    }
    *errors = d.errors;
    return packets;
}

TEST(packet_roundtrip_max_payload)
{
    uint8_t payload[WYE_MAX_PAYLOAD], buf[WYE_MAX_ENCODED];
    for (size_t i = 0; i < sizeof(payload); ++i) {
        payload[i] = (uint8_t)i;
    }
    size_t n = wye_packet_encode(2, 0x32, 7, payload, sizeof(payload), buf, sizeof(buf));
    CHECK(n > 0);
    struct wye_packet p;
    uint32_t errors;
    CHECK_EQ(decode_stream(buf, n, &p, &errors), 1);
    CHECK_EQ(p.node, 2);
    CHECK_EQ(p.type, 0x32);
    CHECK_EQ(p.seq, 7);
    CHECK_EQ(p.len, WYE_MAX_PAYLOAD);
    CHECK_MEM(p.payload, payload, sizeof(payload));
}

TEST(decoder_survives_corruption)
{
    uint8_t stream[4096];
    size_t len = 0;
    for (int i = 0; i < 100; ++i) {
        uint8_t kb[7] = {(uint8_t)i, 4, 5, 0, 0, 0, 0};
        len += wye_packet_encode(2, WYE_MSG_KEYBOARD, (uint8_t)i, kb, 7, stream + len, 64);
    }
    srand(7);
    for (int i = 0; i < 20; ++i) {
        stream[rand() % (int)len] ^= (uint8_t)(1u << (rand() % 8));
    }
    struct wye_packet p;
    uint32_t errors;
    int good = decode_stream(stream, len, &p, &errors);
    CHECK(good >= 60);           /* each flip ruins at most ~2 frames */
    CHECK(good + (int)errors >= 95);
}

TEST(identity_roundtrip_and_truncation)
{
    struct wye_identity id = {0x1209, 0x5759, "Wye", "Wye Target", "A1"}, out;
    uint8_t buf[4 + 3 * WYE_ID_STR_MAX];
    size_t n = wye_identity_pack(&id, buf);
    CHECK(wye_identity_unpack(buf, n, &out));
    CHECK_EQ(out.pid, 0x5759);
    CHECK(strcmp(out.product, "Wye Target") == 0);
    CHECK(!wye_identity_unpack(buf, n - 1, &out));
}
