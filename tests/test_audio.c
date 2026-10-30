/* Audio between the Picos: routing through the simulated bridge and target. */
#include "sim.h"
#include "test.h"

static struct sim_device g_audio_sim;
static uint8_t g_input[WYE_AUDIO_CHUNK], g_output[WYE_AUDIO_CHUNK];
static size_t g_input_len, g_output_len;

static void input_rx(void *ctx, const uint8_t *data, size_t len)
{
    (void)ctx;
    memcpy(g_input, data, len);
    g_input_len = len;
}

static void output_rx(void *ctx, const uint8_t *data, size_t len)
{
    (void)ctx;
    memcpy(g_output, data, len);
    g_output_len = len;
}

static void setup(void)
{
    sim_init(&g_audio_sim);
    g_audio_sim.target_hooks.audio_rx = input_rx;
    g_audio_sim.bridge_hooks.audio_rx = output_rx;
    g_input_len = g_output_len = 0;
}

TEST(audio_duplex_routes_without_cdc_host)
{
    setup();
    struct sim_device *s = &g_audio_sim;
    s->host_present = false; /* App/CDC not open: native USB audio still works. */
    uint8_t pcm[WYE_AUDIO_CHUNK], wire[WYE_MAX_ENCODED];
    for (size_t i = 0; i < sizeof(pcm); ++i) pcm[i] = (uint8_t)i;
    size_t n = wye_packet_encode(WYE_NODE_TARGET, WYE_MSG_AUDIO, 0, pcm, sizeof(pcm), wire, sizeof(wire));
    /* Fragment both directions, as real UART reads may split anywhere. */
    for (size_t i = 0; i < n; ++i) s->bridge_io.uart_write(s, wire + i, 1);
    CHECK_EQ(g_input_len, sizeof(pcm));
    CHECK_MEM(g_input, pcm, sizeof(pcm));
    for (size_t i = 0; i < n; ++i) wye_bridge_uart_rx(&s->bridge, wire + i, 1);
    CHECK_EQ(g_output_len, sizeof(pcm));
    CHECK_MEM(g_output, pcm, sizeof(pcm));
    CHECK_EQ(s->to_host_len, 0);
}

TEST(audio_does_not_leak_to_cdc_or_break_control_packets)
{
    setup();
    struct sim_device *s = &g_audio_sim;
    uint8_t pcm[WYE_AUDIO_FRAME_BYTES] = {0};
    wye_node_send(&s->target.node, WYE_MSG_AUDIO, 0, pcm, sizeof(pcm));
    CHECK_EQ(g_output_len, sizeof(pcm));
    CHECK_EQ(s->to_host_len, 0);
    /* Older/no-audio bridge hooks must also swallow audio. */
    s->bridge_hooks.audio_rx = NULL;
    wye_node_send(&s->target.node, WYE_MSG_AUDIO, 0, pcm, sizeof(pcm));
    CHECK_EQ(s->to_host_len, 0);
    uint8_t leds = WYE_LED_CAPS;
    wye_node_send(&s->target.node, WYE_MSG_LEDS, 19, &leds, 1);
    struct wye_decoder decoder;
    struct wye_packet pkt;
    wye_decoder_init(&decoder);
    int received = 0;
    for (size_t i = 0; i < s->to_host_len; ++i) {
        if (wye_decoder_feed(&decoder, s->to_host[i], &pkt) == WYE_DECODE_PACKET) {
            received++;
            CHECK_EQ(pkt.type, WYE_MSG_LEDS);
            CHECK_EQ(pkt.seq, 19);
            CHECK_EQ(pkt.payload[0], leds);
        }
    }
    CHECK_EQ(received, 1);
}

TEST(audio_rejects_invalid_lengths_and_corrupt_frames)
{
    setup();
    struct sim_device *s = &g_audio_sim;
    uint8_t pcm[WYE_AUDIO_CHUNK + 2] = {0};
    const size_t invalid[] = {0, 1, 3, sizeof(pcm)};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        sim_send(s, WYE_NODE_TARGET, WYE_MSG_AUDIO, pcm, invalid[i]);
        wye_node_send(&s->target.node, WYE_MSG_AUDIO, 0, pcm, invalid[i]);
    }
    CHECK_EQ(g_input_len, 0);
    CHECK_EQ(g_output_len, 0);
    CHECK_EQ(s->to_host_len, 0);
    uint8_t wire[WYE_MAX_ENCODED];
    size_t n = wye_packet_encode(WYE_NODE_TARGET, WYE_MSG_AUDIO, 0, pcm, 32, wire, sizeof(wire));
    wire[n - 2] ^= 0x40; /* corrupt CRC */
    wye_bridge_uart_rx(&s->bridge, wire, n);
    CHECK_EQ(g_output_len, 0);
    CHECK_EQ(s->to_host_len, 0);
    CHECK_EQ(s->bridge.node.rx_errors, 1);
    wye_node_send(&s->target.node, WYE_MSG_AUDIO, 0, pcm, 32);
    CHECK_EQ(g_output_len, 32); /* resynchronises */
}
