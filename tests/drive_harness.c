/* Drive tests: the host drive wired to the simulated target through the bridge relay. */
#include "drive_harness.h"
#include "plat_thread.h"

struct sim_device g_sim;
struct wye_drive g_drive;
bool g_saw_removing;

static bool to_target(void *ctx, uint8_t type, const void *payload, size_t len)
{
    (void)ctx;
    sim_send(&g_sim, WYE_NODE_TARGET, type, payload, len);
    return true;
}

static const struct wye_sender SENDER = {NULL, to_target};

/* Moves host-bound traffic into the drive and lets time pass. */
void pump(uint32_t ms)
{
    uint8_t buf[8192];
    struct wye_decoder d;
    struct wye_packet p;
    wye_decoder_init(&d);
    for (uint32_t i = 0; i < ms; ++i) {
        sim_advance(&g_sim, 1);
        size_t n = sim_host_read(&g_sim, buf, sizeof(buf));
        for (size_t k = 0; k < n; ++k) {
            if (wye_decoder_feed(&d, buf[k], &p) == WYE_DECODE_PACKET && p.node == WYE_NODE_TARGET) {
                wye_drive_packet(&g_drive, &p, &SENDER);
            }
        }
        wye_drive_tick(&g_drive, g_sim.now, true, true, &SENDER);
        g_saw_removing |= g_drive.state == WYE_DRIVE_REMOVING;
    }
}

void wait_state(enum wye_drive_state s)
{
    for (int i = 0; i < 2000 && g_drive.state != s; ++i) {
        pump(5);
        plat_sleep_ms(1); /* packing runs on a real worker thread */
    }
}
