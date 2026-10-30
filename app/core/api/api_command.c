/* The commands of the local API. */
#include "api.h"

#include "plat_thread.h"
#include "wye_proto.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WHEEL_MAX 127

/* Buttons held by `down`, kept through moves until `up`. */
static uint8_t g_buttons;

static void reply_err(char *reply, size_t cap, const char *code, const char *message)
{
    snprintf(reply, cap, "err %s %s", code, message);
}

void wye_api_escape(const char *in, char *out, size_t cap)
{
    size_t o = 0;
    for (; *in && o + 3 < cap; ++in) {
        char e = *in == '\n' ? 'n' : *in == '\t' ? 't' : *in == '\r' ? 'r' : *in == '\\' ? '\\' : 0;
        if (e) {
            out[o++] = '\\';
            out[o++] = e;
        } else {
            out[o++] = *in;
        }
    }
    out[o] = '\0';
}

void wye_api_unescape(char *text)
{
    char *o = text;
    for (char *s = text; *s; ++s) {
        if (*s == '\\' && s[1]) {
            ++s;
            *o++ = *s == 'n' ? '\n' : *s == 't' ? '\t' : *s == 'r' ? '\r' : *s;
        } else {
            *o++ = *s;
        }
    }
    *o = '\0';
}

static bool parse_int(const char *s, int lo, int hi, int *out)
{
    char *end;
    errno = 0;
    long v = s ? strtol(s, &end, 10) : 0;
    if (!s || !*s || *end || errno || v < lo || v > hi) {
        return false;
    }
    *out = (int)v;
    return true;
}

static bool parse_button(const char *s, uint8_t *bit)
{
    static const struct {
        const char *name;
        uint8_t bit;
    } B[] = {{"left", WYE_BTN_LEFT}, {"right", WYE_BTN_RIGHT}, {"middle", WYE_BTN_MIDDLE},
             {"back", WYE_BTN_BACK}, {"forward", WYE_BTN_FORWARD}};
    if (!s) {
        *bit = WYE_BTN_LEFT;
        return true;
    }
    for (size_t i = 0; i < sizeof(B) / sizeof(B[0]); ++i) {
        if (strcmp(s, B[i].name) == 0) {
            *bit = B[i].bit;
            return true;
        }
    }
    return false;
}

/* Replies only once the keys are out, so a following mouse command cannot overtake them. */
static bool wait_typed(const struct wye_api_ops *ops, char *reply, size_t cap, const volatile bool *cancel)
{
    while (ops->typing_busy()) {
        if (cancel && *cancel) {
            reply_err(reply, cap, "not-ready", "Wye is closing");
            return false;
        }
        plat_sleep_ms(5);
    }
    if (ops->state() == WYE_API_NOT_READY) { /* a lost link empties the queue */
        reply_err(reply, cap, "not-ready", "the other computer went away");
        return false;
    }
    return true;
}

static void cmd_type(const struct wye_api_ops *ops, char *text, char *reply, size_t cap, const volatile bool *cancel)
{
    wye_api_unescape(text);
    size_t total = strlen(text), typed = ops->type_text(text);
    if (wait_typed(ops, reply, cap, cancel)) {
        snprintf(reply, cap, "ok typed=%lu skipped=%lu", (unsigned long)typed, (unsigned long)(total - typed));
    }
}

static void cmd_key(const struct wye_api_ops *ops, char *args, char *reply, size_t cap, const volatile bool *cancel)
{
    uint8_t mods[32], keys[32];
    int n = 0;
    for (char *t = strtok(args, " "); t; t = strtok(NULL, " ")) {
        if (n == 32 || !wye_api_parse_chord(t, &mods[n], &keys[n])) {
            snprintf(reply, cap, "err bad-request unknown key: %.64s", t);
            return;
        }
        n++;
    }
    if (n == 0) {
        reply_err(reply, cap, "bad-request", "usage: key <chord> [chord...]");
        return;
    }
    for (int i = 0; i < n; ++i) {
        ops->chord(mods[i], keys[i]);
    }
    if (wait_typed(ops, reply, cap, cancel)) {
        snprintf(reply, cap, "ok");
    }
}

static void cmd_mouse(const struct wye_api_ops *ops, const char *cmd, char *args, char *reply, size_t cap)
{
    char *a = strtok(args, " "), *b = strtok(NULL, " ");
    int x = 0, y = 0;
    uint8_t bit = 0;
    bool ok = false;
    if (strcmp(cmd, "move") == 0) {
        if (!parse_int(a, -32768, 32767, &x) || !parse_int(b, -32768, 32767, &y)) {
            reply_err(reply, cap, "bad-request", "usage: move <dx> <dy>");
            return;
        }
        ok = ops->mouse(g_buttons, x, y, 0, 0);
    } else if (strcmp(cmd, "moveto") == 0) {
        if (!parse_int(a, 0, WYE_ABS_MAX, &x) || !parse_int(b, 0, WYE_ABS_MAX, &y)) {
            reply_err(reply, cap, "bad-request", "usage: moveto <x> <y> (0-32767 across the screen)");
            return;
        }
        if (!ops->mouse_abs(g_buttons, x, y)) {
            reply_err(reply, cap, "unsupported", "this firmware has no absolute pointer");
            return;
        }
        ok = true;
    } else if (strcmp(cmd, "scroll") == 0) {
        if (!parse_int(a, -WHEEL_MAX, WHEEL_MAX, &y) || (b && !parse_int(b, -WHEEL_MAX, WHEEL_MAX, &x))) {
            reply_err(reply, cap, "bad-request", "usage: scroll <notches up> [notches right] (-127 to 127)");
            return;
        }
        ok = ops->mouse(g_buttons, 0, 0, y, x);
    } else {
        if (!parse_button(a, &bit) || b) {
            reply_err(reply, cap, "bad-request", "button: left, right, middle, back or forward");
            return;
        }
        if (strcmp(cmd, "click") == 0) {
            ok = ops->mouse(g_buttons | bit, 0, 0, 0, 0) && ops->mouse(g_buttons, 0, 0, 0, 0);
        } else {
            g_buttons = strcmp(cmd, "down") == 0 ? (uint8_t)(g_buttons | bit) : (uint8_t)(g_buttons & ~bit);
            ok = ops->mouse(g_buttons, 0, 0, 0, 0);
        }
    }
    if (ok) {
        snprintf(reply, cap, "ok");
    } else {
        reply_err(reply, cap, "not-ready", "the other computer is not connected");
    }
}

void wye_api_command(const struct wye_api_ops *ops, char *line, char *reply, size_t cap, const volatile bool *cancel)
{
    char *args = strchr(line, ' ');
    if (args) {
        *args++ = '\0';
    } else {
        args = line + strlen(line);
    }
    if (strcmp(line, "status") == 0) {
        char s[256];
        ops->status(s, sizeof(s));
        snprintf(reply, cap, "ok %s", s);
        return;
    }
    if (strcmp(line, "release") == 0) {
        g_buttons = 0;
        ops->release_all();
        snprintf(reply, cap, "ok");
        return;
    }
    static const char *const INPUT[] = {"type", "key", "move", "moveto", "click", "down", "up", "scroll"};
    bool input = false;
    for (size_t i = 0; i < sizeof(INPUT) / sizeof(INPUT[0]); ++i) {
        input = input || strcmp(line, INPUT[i]) == 0;
    }
    if (!input) {
        snprintf(reply, cap, "err bad-request unknown command: %.64s", line);
        return;
    }
    enum wye_api_state st = ops->state();
    if (st == WYE_API_NOT_READY) {
        reply_err(reply, cap, "not-ready", "the other computer is not connected");
    } else if (st == WYE_API_BUSY) {
        reply_err(reply, cap, "busy", "someone is controlling the other computer, or a routine is playing");
    } else if (strcmp(line, "type") == 0) {
        cmd_type(ops, args, reply, cap, cancel);
    } else if (strcmp(line, "key") == 0) {
        cmd_key(ops, args, reply, cap, cancel);
    } else {
        cmd_mouse(ops, line, args, reply, cap);
    }
}
