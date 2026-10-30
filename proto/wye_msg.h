/* Typed payloads for WYE_MSG_* and their (de)serialisers. */
#ifndef WYE_MSG_H
#define WYE_MSG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct wye_hello_ack {
    uint8_t proto_version;
    uint8_t role;              /* WYE_ROLE_* */
    uint8_t fw_major, fw_minor, fw_patch;
    uint8_t board;             /* 1 = RP2040, 2 = RP2350 */
    uint16_t caps;             /* WYE_CAP_* */
    uint8_t uid[8];            /* flash unique id */
    uint32_t build_id;         /* short git hash */
};
#define WYE_HELLO_ACK_LEN 20u

struct wye_pong {
    uint32_t token;
    uint8_t status;            /* WYE_TSTATE_* or WYE_BSTATE_* */
    uint8_t leds;
    uint16_t rx_errors;        /* saturating */
    uint32_t uptime_ms;
};
#define WYE_PONG_LEN 12u

struct wye_keyboard {
    uint8_t modifiers;
    uint8_t keys[6];
};
#define WYE_KEYBOARD_LEN 7u

struct wye_mouse {
    uint8_t buttons;
    int16_t dx, dy;
    int8_t wheel, pan;
};
#define WYE_MOUSE_LEN 7u

struct wye_abs_mouse {
    uint8_t buttons;
    uint16_t x, y;             /* 0..WYE_ABS_MAX */
    int8_t wheel, pan;
};
#define WYE_ABS_MOUSE_LEN 7u

#define WYE_ID_STR_MAX 32u
struct wye_identity {
    uint16_t vid, pid;
    char manufacturer[WYE_ID_STR_MAX];
    char product[WYE_ID_STR_MAX];
    char serial[WYE_ID_STR_MAX];
};

/* Target USB functions: shared drive, duplex audio, and the drive's names. */
#define WYE_FN_NAME_MAX 32u
enum wye_fn_flag { WYE_FN_DRIVE = 1u << 0, WYE_FN_AUDIO = 1u << 1 };
struct wye_functions {
    uint8_t flags;                       /* WYE_FN_* */
    char drive_vendor[9], drive_product[17];  /* SCSI inquiry: what file managers show */
};

/* What you call a board. Lives in the board's flash, so it travels with it. */
#define WYE_PROFILE_NAME_MAX 24u    /* bytes of UTF-8 */
#define WYE_PROFILE_NOTES_MAX 64u
#define WYE_PROFILE_COLORS 7u       /* 0: the role's own colour */
struct wye_profile {
    uint8_t color;
    char name[WYE_PROFILE_NAME_MAX + 1];    /* empty: the role's own name */
    char notes[WYE_PROFILE_NOTES_MAX + 1];
};
#define WYE_PROFILE_PACKED_MAX (3u + WYE_PROFILE_NAME_MAX + WYE_PROFILE_NOTES_MAX)

static inline void wye_put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static inline uint16_t wye_get_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static inline void wye_put_u32(uint8_t *p, uint32_t v)
{
    wye_put_u16(p, (uint16_t)v);
    wye_put_u16(p + 2, (uint16_t)(v >> 16));
}
static inline uint32_t wye_get_u32(const uint8_t *p)
{
    return (uint32_t)wye_get_u16(p) | ((uint32_t)wye_get_u16(p + 2) << 16);
}

/* Pack functions return bytes written; unpack functions validate length. */
size_t wye_hello_ack_pack(const struct wye_hello_ack *a, uint8_t *out);
bool wye_hello_ack_unpack(const uint8_t *p, size_t len, struct wye_hello_ack *a);
size_t wye_pong_pack(const struct wye_pong *a, uint8_t *out);
bool wye_pong_unpack(const uint8_t *p, size_t len, struct wye_pong *a);
size_t wye_keyboard_pack(const struct wye_keyboard *k, uint8_t *out);
bool wye_keyboard_unpack(const uint8_t *p, size_t len, struct wye_keyboard *k);
size_t wye_mouse_pack(const struct wye_mouse *m, uint8_t *out);
bool wye_mouse_unpack(const uint8_t *p, size_t len, struct wye_mouse *m);
size_t wye_abs_mouse_pack(const struct wye_abs_mouse *m, uint8_t *out);
bool wye_abs_mouse_unpack(const uint8_t *p, size_t len, struct wye_abs_mouse *m);
/* out must hold at least 4 + 3 * WYE_ID_STR_MAX bytes. */
size_t wye_identity_pack(const struct wye_identity *id, uint8_t *out);
bool wye_identity_unpack(const uint8_t *p, size_t len, struct wye_identity *id);
/* out must hold at least 1 + 2 * WYE_FN_NAME_MAX bytes. */
size_t wye_functions_pack(const struct wye_functions *f, uint8_t *out);
bool wye_functions_unpack(const uint8_t *p, size_t len, struct wye_functions *f);
size_t wye_profile_pack(const struct wye_profile *f, uint8_t *out);
bool wye_profile_unpack(const uint8_t *p, size_t len, struct wye_profile *f);
/* One line, no surrounding spaces, cut to max bytes on a character boundary. */
void wye_profile_clean(char *text, size_t max);

#endif
