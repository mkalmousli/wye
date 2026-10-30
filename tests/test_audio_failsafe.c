/* Audio keeps flowing, but never counts as input: held keys are still released. */
#include "sim.h"
#include "test.h"

static struct sim_device g_sim;
static size_t g_input_len;

static void input_rx(void *ctx, const uint8_t *data, size_t len)
{
    (void)ctx; (void)data;
    g_input_len = len;
}

TEST(audio_stream_does_not_defeat_input_failsafe)
{
    struct sim_device *s = &g_sim;
    sim_init(s);
    s->target_hooks.audio_rx = input_rx;
    struct wye_keyboard keys = {0, {4}};
    uint8_t payload[WYE_KEYBOARD_LEN], pcm[WYE_AUDIO_FRAME_BYTES] = {0};
    sim_send(s, WYE_NODE_TARGET, WYE_MSG_KEYBOARD, payload, wye_keyboard_pack(&keys, payload));
    sim_advance(s, 1);
    CHECK_EQ(s->reports[0].kbd.keys[0], 4);
    for (unsigned i = 0; i < WYE_TARGET_FAILSAFE_MS + 10; ++i) {
        sim_send(s, WYE_NODE_TARGET, WYE_MSG_AUDIO, pcm, sizeof(pcm));
        sim_advance(s, 1);
    }
    CHECK(s->target.failsafe_tripped);
    CHECK_EQ(s->reports[s->report_count - 1].kbd.keys[0], 0);
    CHECK_EQ(g_input_len, sizeof(pcm));
}
