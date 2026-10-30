/*
 * User settings, stored as key=value text in the per-user config directory.
 * Every field is editable at runtime from the Settings page.
 */
#ifndef WYE_CONFIG_H
#define WYE_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum wye_update_mode { WYE_UPDATE_OFF = 0, WYE_UPDATE_NOTIFY, WYE_UPDATE_AUTO };
enum wye_hud_mode { WYE_HUD_NEVER = 0, WYE_HUD_FORWARDING };

#define WYE_CFG_STR 512

struct wye_config {
    int hotkey;                 /* HID usage, default F12 */
    int sensitivity;            /* percent, 10..400 */
    bool invert_wheel;
    int update_mode;            /* enum wye_update_mode */
    bool api_enabled;           /* other apps of this user may drive the target */
    bool tray_enabled;          /* an icon in the system tray */
    bool auto_firmware;         /* update Picos automatically when idle */
    bool start_at_login;
    bool start_hidden;
    int hud_mode;               /* WYE_HUD_FORWARDING: only the mini strip while controlling */
    bool probe_generic_pico;    /* look for old-firmware Picos */
    int jiggler_interval_s;
    char update_url[WYE_CFG_STR];
    char bridge_ids[128];       /* extra "vid:pid,..." identities to treat as bridges */
    bool drive_enabled;         /* serve the shared folder to the target */
    int drive_size_mb;          /* shared drive capacity, 16..65536 */
    char drive_folder[WYE_CFG_STR]; /* empty: ~/Wye Drive */
    bool last_mini;             /* the mini panel was showing when Wye last ran */
    int mini_x, mini_y;         /* where the mini panel was (both 0: not placed yet) */
    bool routine_compress;      /* tidy every new recording when it stops */
    bool use_keyboard, use_mouse; /* what the other computer gets while you control it */
    bool use_speaker;           /* hear the other computer's sound here */
    bool use_mic;               /* this computer's microphone becomes the other computer's */
    unsigned char shortcuts[16]; /* hotkey + key per WYE_ACT_* (HID usage, 0 = none) */
    int64_t first_run;          /* unix time; 0 opens the setup wizard */
    int64_t last_update_check;
};

void wye_config_defaults(struct wye_config *c);
/* Loads from the config dir (defaults for missing keys); false if unreadable. */
bool wye_config_load(struct wye_config *c);
bool wye_config_save(const struct wye_config *c);
/* Clamps values into valid ranges. */
void wye_config_sanitize(struct wye_config *c);

/* Internal: field table shared by load/save (config_fields.c). */
enum wye_cfg_type { WYE_CFG_INT, WYE_CFG_BOOL, WYE_CFG_I64, WYE_CFG_TEXT, WYE_CFG_HEX };
struct wye_cfg_field {
    const char *key;
    enum wye_cfg_type type;
    size_t offset;
    size_t size;
};
const struct wye_cfg_field *wye_config_fields(int *count);

#endif
