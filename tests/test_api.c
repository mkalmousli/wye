/* The local API: chords, escaping, commands against a fake engine, and a real socket. */
#define _DEFAULT_SOURCE /* setenv */
#include "api/api.h"
#include "hid_usage.h"
#include "plat_ipc.h"
#include "plat_thread.h"
#include "test.h"
#include "wye_proto.h"

#include <stdio.h>
#include <stdlib.h>

static enum wye_api_state g_state;
static char g_typed[256];
static uint8_t g_mods, g_key;
static int g_chords, g_mouse_calls, g_abs_calls, g_releases, g_busy_polls;
static struct { uint8_t buttons; int dx, dy, wheel, pan; } g_last;
static bool g_abs_ok;

static enum wye_api_state f_state(void) { return g_state; }
static size_t f_type(const char *t)
{
    size_t n = 0;
    for (const char *s = t; *s; ++s) {
        if ((unsigned char)*s < 0x80) {
            g_typed[n++] = *s;
        }
    }
    g_typed[n] = '\0';
    g_busy_polls = 2;
    return n;
}
static bool f_chord(uint8_t m, uint8_t k) { g_mods = m; g_key = k; g_chords++; return true; }
static bool f_busy(void) { return g_busy_polls-- > 0; }
static bool f_mouse(uint8_t b, int dx, int dy, int w, int p)
{
    g_last.buttons = b; g_last.dx = dx; g_last.dy = dy; g_last.wheel = w; g_last.pan = p;
    g_mouse_calls++;
    return true;
}
static bool f_abs(uint8_t b, int x, int y) { (void)b; g_last.dx = x; g_last.dy = y; g_abs_calls++; return g_abs_ok; }
static void f_release(void) { g_releases++; }
static void f_status(char *o, size_t n) { snprintf(o, n, "target=fake"); }
static const struct wye_api_ops OPS = {f_state, f_type, f_chord, f_busy, f_mouse, f_abs, f_release, f_status};

static char g_reply[512];
static const char *run(const char *cmd)
{
    static char line[512];
    snprintf(line, sizeof(line), "%s", cmd);
    wye_api_command(&OPS, line, g_reply, sizeof(g_reply), NULL);
    return g_reply;
}

static void reset(void)
{
    g_state = WYE_API_READY;
    g_abs_ok = true;
    g_chords = g_mouse_calls = g_abs_calls = g_releases = 0;
    g_typed[0] = '\0';
    run("release");
    g_releases = 0;
}

TEST(api_parses_chords)
{
    uint8_t m, k;
    CHECK(wye_api_parse_chord("ctrl+shift+t", &m, &k));
    CHECK_EQ(m, 0x03);
    CHECK_EQ(k, HID_KEY_A + ('t' - 'a'));
    CHECK(wye_api_parse_chord("CTRL+ALT+Delete", &m, &k) && m == 0x05 && k == HID_KEY_DELETE);
    CHECK(wye_api_parse_chord("f12", &m, &k) && m == 0 && k == HID_KEY_F12);
    CHECK(wye_api_parse_chord("f24", &m, &k) && k == HID_KEY_F24);
    CHECK(wye_api_parse_chord("win", &m, &k) && m == 0x08 && k == 0);
    CHECK(wye_api_parse_chord("A", &m, &k) && m == 0x02 && k == HID_KEY_A);
    CHECK(wye_api_parse_chord("ctrl+/", &m, &k) && m == 0x01 && k == 0x38);
    CHECK(wye_api_parse_chord("+", &m, &k) && m == 0x02 && k == 0x2e);
    CHECK(wye_api_parse_chord("ctrl++", &m, &k) && m == 0x03 && k == 0x2e);
    CHECK(!wye_api_parse_chord("", &m, &k));
    CHECK(!wye_api_parse_chord("nosuchkey", &m, &k));
    CHECK(!wye_api_parse_chord("a+b", &m, &k));
    CHECK(!wye_api_parse_chord("f25", &m, &k));
    CHECK(!wye_api_parse_chord("ctrl+", &m, &k));
}

TEST(api_escape_round_trips)
{
    char out[64];
    wye_api_escape("a\nb\tc\\d", out, sizeof(out));
    CHECK(strcmp(out, "a\\nb\\tc\\\\d") == 0);
    CHECK(!strchr(out, '\n'));
    wye_api_unescape(out);
    CHECK(strcmp(out, "a\nb\tc\\d") == 0);
}

TEST(api_types_and_presses_when_ready)
{
    reset();
    CHECK(strcmp(run("type hello\\nworld"), "ok typed=11 skipped=0") == 0);
    CHECK(strcmp(g_typed, "hello\nworld") == 0);
    CHECK_EQ(g_busy_polls, -1); /* the reply waited for the keys to go out */
    CHECK(strcmp(run("type a\xc3\xa4"), "ok typed=1 skipped=2") == 0);
    CHECK(strcmp(run("key ctrl+l enter"), "ok") == 0);
    CHECK_EQ(g_chords, 2);
    CHECK_EQ(g_key, HID_KEY_ENTER);
    g_chords = 0;
    CHECK(strncmp(run("key ctrl+l nosuchkey"), "err bad-request", 15) == 0);
    CHECK_EQ(g_chords, 0); /* all or nothing */
    CHECK(strncmp(run("key"), "err bad-request", 15) == 0);
}

TEST(api_mouse_commands)
{
    reset();
    CHECK(strcmp(run("move 10 -5"), "ok") == 0);
    CHECK(g_last.dx == 10 && g_last.dy == -5 && g_last.buttons == 0);
    CHECK(strcmp(run("click right"), "ok") == 0);
    CHECK_EQ(g_mouse_calls, 3); /* press and release */
    CHECK_EQ(g_last.buttons, 0);
    CHECK(strcmp(run("down"), "ok") == 0);
    CHECK_EQ(g_last.buttons, WYE_BTN_LEFT);
    run("move 1 1");
    CHECK_EQ(g_last.buttons, WYE_BTN_LEFT); /* a drag keeps it held */
    run("click right");
    CHECK_EQ(g_last.buttons, WYE_BTN_LEFT);
    run("up");
    CHECK_EQ(g_last.buttons, 0);
    CHECK(strcmp(run("scroll -2 3"), "ok") == 0);
    CHECK(g_last.wheel == -2 && g_last.pan == 3);
    CHECK(strcmp(run("moveto 100 200"), "ok") == 0);
    CHECK_EQ(g_abs_calls, 1);
    g_abs_ok = false;
    CHECK(strncmp(run("moveto 1 2"), "err unsupported", 15) == 0);
    int calls = g_mouse_calls;
    CHECK(strncmp(run("move 99999 0"), "err bad-request", 15) == 0);
    CHECK(strncmp(run("move 1"), "err bad-request", 15) == 0);
    CHECK(strncmp(run("moveto 40000 0"), "err bad-request", 15) == 0);
    CHECK(strncmp(run("scroll 500"), "err bad-request", 15) == 0);
    CHECK(strncmp(run("click nose"), "err bad-request", 15) == 0);
    CHECK_EQ(g_mouse_calls, calls);
}

TEST(api_refuses_when_not_ready_or_user_in_control)
{
    reset();
    g_state = WYE_API_NOT_READY;
    CHECK(strncmp(run("type x"), "err not-ready", 13) == 0);
    CHECK(strncmp(run("click"), "err not-ready", 13) == 0);
    g_state = WYE_API_BUSY;
    CHECK(strncmp(run("key enter"), "err busy", 8) == 0);
    CHECK(strncmp(run("move 1 1"), "err busy", 8) == 0);
    CHECK_EQ(g_chords, 0);
    CHECK_EQ(g_mouse_calls, 0);
    CHECK(strcmp(run("status"), "ok target=fake") == 0); /* always answers */
    CHECK(strcmp(run("release"), "ok") == 0);
    CHECK_EQ(g_releases, 1);
    CHECK(strncmp(run("nonsense"), "err bad-request", 15) == 0);
}

static bool ask(plat_ipc *c, const char *line, const char *want)
{
    char got[1024];
    size_t used = 0;
    if (*line && !plat_ipc_write(c, line, strlen(line))) {
        return false;
    }
    while (used < sizeof(got) - 1) {
        int n = plat_ipc_read(c, got + used, sizeof(got) - 1 - used, 2000);
        if (n <= 0) {
            return false;
        }
        used += (size_t)n;
        got[used] = '\0';
        if (strchr(got, '\n')) {
            return strncmp(got, want, strlen(want)) == 0;
        }
    }
    return false;
}

TEST(api_server_over_a_real_socket)
{
    reset();
#if defined(_WIN32)
    _putenv_s("WYE_API_SOCKET", "\\\\.\\pipe\\wye-test");
#else
    setenv("WYE_API_SOCKET", "out/test-api.sock", 1);
#endif
    CHECK(wye_api_start(&OPS));
    plat_ipc *a = plat_ipc_connect(), *b = plat_ipc_connect();
    CHECK(a && b);
    CHECK(ask(a, "status\n", "ok target=fake\n"));
    CHECK(ask(b, "status\r\n", "ok target=fake\n")); /* several clients, CRLF too */
    CHECK(ask(a, "status\n", "ok target=fake\n"));
    CHECK(ask(a, "move 3 4\n", "ok\n"));
    CHECK(g_last.dx == 3 && g_last.dy == 4);
    static char huge[WYE_API_LINE * 2];
    memset(huge, 'x', sizeof(huge) - 2);
    huge[sizeof(huge) - 2] = '\n';
    CHECK(plat_ipc_write(b, huge, sizeof(huge) - 1));
    CHECK(ask(b, "", "err bad-request line too long"));
    plat_ipc_close(a);
    plat_ipc_close(b);
    plat_sleep_ms(50);
    plat_ipc *d = plat_ipc_connect(); /* a client leaving frees its place */
    CHECK(d && ask(d, "status\n", "ok target=fake\n"));
    plat_ipc_close(d);
    wye_api_stop();
    CHECK(plat_ipc_connect() == NULL); /* and the socket is gone */
#if defined(_WIN32)
    _putenv_s("WYE_API_SOCKET", "");
#else
    unsetenv("WYE_API_SOCKET");
#endif
}
