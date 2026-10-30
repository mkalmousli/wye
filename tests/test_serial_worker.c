#include "link/serial_worker.h"
#include "test.h"

static struct {
    plat_event *opening, *allow_open, *closing, *allow_close;
    bool fail;
} g_serial;

static plat_serial *slow_open(const char *path, char *err, size_t len)
{
    (void)path;
    plat_event_signal(g_serial.opening);
    plat_event_wait(g_serial.allow_open, 3000);
    if (g_serial.fail) {
        snprintf(err, len, "device did not respond");
        return NULL;
    }
    return (plat_serial *)&g_serial;
}

static void slow_close(plat_serial *port)
{
    (void)port;
    plat_event_signal(g_serial.closing);
    plat_event_wait(g_serial.allow_close, 3000);
}

TEST(serial_open_and_close_do_not_block_engine)
{
    g_serial.opening = plat_event_new();
    g_serial.allow_open = plat_event_new();
    g_serial.closing = plat_event_new();
    g_serial.allow_close = plat_event_new();
    g_serial.fail = false;
    plat_event *wake = plat_event_new();
    const struct wye_serial_ops ops = {slow_open, slow_close};
    struct wye_serial_worker *w = wye_serial_worker_new(wake, &ops);
    char err[128] = "";
    uint64_t start = plat_now_ms();
    void *first = wye_serial_worker_open(w, "SIM0", err, sizeof(err));
    bool fast_open = plat_now_ms() - start < 250;
    bool opening = plat_event_wait(g_serial.opening, 1000);
    bool pending = wye_serial_worker_pending(w);
    /* The fake driver cannot return until we release it. Polls must return. */
    start = plat_now_ms();
    for (int i = 0; i < 100; ++i) wye_serial_worker_open(w, "SIM0", err, sizeof(err));
    bool fast_poll = plat_now_ms() - start < 250;
    plat_event_signal(g_serial.allow_open);
    bool completed = plat_event_wait(wake, 1000);
    void *port = wye_serial_worker_open(w, "SIM0", err, sizeof(err));
    bool adopted = !wye_serial_worker_pending(w);
    start = plat_now_ms();
    wye_serial_worker_close(w, port);
    bool fast_close = plat_now_ms() - start < 250;
    bool closing = plat_event_wait(g_serial.closing, 1000);
    plat_event_signal(g_serial.allow_close);
    wye_serial_worker_free(w);
    plat_event_free(wake);
    plat_event_free(g_serial.opening);
    plat_event_free(g_serial.allow_open);
    plat_event_free(g_serial.closing);
    plat_event_free(g_serial.allow_close);
    CHECK(first == NULL && opening && pending && completed && adopted && closing);
    CHECK(port == (void *)&g_serial);
    CHECK(fast_open && fast_poll && fast_close);
}

TEST(serial_worker_reports_background_open_failure)
{
    g_serial.opening = plat_event_new();
    g_serial.allow_open = plat_event_new();
    g_serial.fail = true;
    plat_event *wake = plat_event_new();
    const struct wye_serial_ops ops = {slow_open, slow_close};
    struct wye_serial_worker *w = wye_serial_worker_new(wake, &ops);
    char err[128] = "";
    wye_serial_worker_open(w, "SIM0", err, sizeof(err));
    plat_event_signal(g_serial.allow_open);
    bool completed = plat_event_wait(wake, 1000);
    void *port = wye_serial_worker_open(w, "SIM0", err, sizeof(err));
    bool pending = wye_serial_worker_pending(w);
    wye_serial_worker_free(w);
    plat_event_free(wake);
    plat_event_free(g_serial.opening);
    plat_event_free(g_serial.allow_open);
    CHECK(completed && !port && !pending);
    CHECK(strcmp(err, "device did not respond") == 0);
}
