/*
 * Local API: lets other programs of this user drive the target directly, with
 * no hotkey and without touching this computer's own keyboard and mouse.
 *
 * Newline-terminated text commands over a Unix socket or named pipe; each gets
 * one reply line, "ok ..." or "err <code> <message>". See docs/API.md.
 */
#ifndef WYE_API_H
#define WYE_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WYE_API_LINE 8192u

enum wye_api_state {
    WYE_API_READY = 0,
    WYE_API_NOT_READY,  /* the target is not connected */
    WYE_API_BUSY,       /* the user is controlling it, or a routine is playing */
};

/* What commands act on: the engine in the app, a fake in the tests. */
struct wye_api_ops {
    enum wye_api_state (*state)(void);
    size_t (*type_text)(const char *text);      /* characters queued */
    bool (*chord)(uint8_t modifiers, uint8_t key);
    bool (*typing_busy)(void);
    bool (*mouse)(uint8_t buttons, int dx, int dy, int wheel, int pan);
    bool (*mouse_abs)(uint8_t buttons, int x, int y);   /* false: the target cannot do it */
    void (*release_all)(void);
    void (*status)(char *out, size_t len);      /* "key=value ..." */
};

/* Runs one command line (no newline) and writes the reply (no newline). `cancel`
   ends a wait for typing to finish. */
void wye_api_command(const struct wye_api_ops *ops, char *line, char *reply, size_t cap, const volatile bool *cancel);

/* "ctrl+shift+a" -> HID modifiers and key (0 for modifiers alone). */
bool wye_api_parse_chord(const char *text, uint8_t *modifiers, uint8_t *key);
void wye_api_escape(const char *in, char *out, size_t cap);     /* for the `type` command */
void wye_api_unescape(char *text);

/* The listener and its client threads. */
bool wye_api_start(const struct wye_api_ops *ops);
void wye_api_stop(void);

/* `wye status`, `wye type ...`: true if argv[1] is a command; *exit_code is set. */
bool wye_api_cli(int argc, char **argv, int *exit_code);

#endif
