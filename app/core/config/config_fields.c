/* Table of persisted settings; adding a setting = one line here. */
#include "config.h"

#include "hid_usage.h"
#include "input/input.h"
#include "product.h"
#include "util/util.h"

#include <stddef.h>
#include <string.h>

#define F(key, type, field) {key, type, offsetof(struct wye_config, field), sizeof(((struct wye_config *)0)->field)}

static const struct wye_cfg_field FIELDS[] = {
    F("hotkey", WYE_CFG_INT, hotkey),
    F("sensitivity", WYE_CFG_INT, sensitivity),
    F("invert_wheel", WYE_CFG_BOOL, invert_wheel),
    F("update_mode", WYE_CFG_INT, update_mode),
    F("auto_firmware", WYE_CFG_BOOL, auto_firmware),
    F("start_at_login", WYE_CFG_BOOL, start_at_login),
    F("start_hidden", WYE_CFG_BOOL, start_hidden),
    F("hud_mode", WYE_CFG_INT, hud_mode),
    F("probe_generic_pico", WYE_CFG_BOOL, probe_generic_pico),
    F("jiggler_interval_s", WYE_CFG_INT, jiggler_interval_s),
    F("update_url", WYE_CFG_TEXT, update_url),
    F("bridge_ids", WYE_CFG_TEXT, bridge_ids),
    F("drive_enabled", WYE_CFG_BOOL, drive_enabled),
    F("drive_size_mb", WYE_CFG_INT, drive_size_mb),
    F("drive_folder", WYE_CFG_TEXT, drive_folder),
    F("start_mini", WYE_CFG_BOOL, start_mini),
    F("use_keyboard", WYE_CFG_BOOL, use_keyboard),
    F("use_mouse", WYE_CFG_BOOL, use_mouse),
    F("use_speaker", WYE_CFG_BOOL, use_speaker),
    F("use_mic", WYE_CFG_BOOL, use_mic),
    F("shortcuts", WYE_CFG_HEX, shortcuts),
    F("first_run", WYE_CFG_I64, first_run),
    F("last_update_check", WYE_CFG_I64, last_update_check),
};

const struct wye_cfg_field *wye_config_fields(int *count)
{
    *count = (int)WYE_ARRAY_LEN(FIELDS);
    return FIELDS;
}

void wye_config_defaults(struct wye_config *c)
{
    memset(c, 0, sizeof(*c));
    c->hotkey = HID_KEY_F12;
    c->sensitivity = 100;
    c->update_mode = WYE_UPDATE_AUTO;
    c->auto_firmware = true;
    c->hud_mode = WYE_HUD_FORWARDING;
    c->probe_generic_pico = true;
    c->jiggler_interval_s = 60;
    c->drive_enabled = true;
    c->drive_size_mb = 256;
    c->start_mini = false;
    c->use_keyboard = c->use_mouse = c->use_speaker = c->use_mic = true;
    uint8_t keys[WYE_ACT_COUNT];
    wye_input_default_chords(keys);
    memcpy(c->shortcuts, keys, sizeof(keys));
    wye_strcpy(c->update_url, sizeof(c->update_url), WYE_UPDATE_URL);
}

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

void wye_config_sanitize(struct wye_config *c)
{
    c->hotkey = clampi(c->hotkey, HID_KEY_A, HID_KEY_RIGHT_GUI);
    c->sensitivity = clampi(c->sensitivity, 10, 400);
    c->update_mode = clampi(c->update_mode, WYE_UPDATE_OFF, WYE_UPDATE_AUTO);
    c->hud_mode = clampi(c->hud_mode, WYE_HUD_NEVER, WYE_HUD_FORWARDING);
    c->jiggler_interval_s = clampi(c->jiggler_interval_s, 10, 3600);
    c->drive_size_mb = clampi(c->drive_size_mb, 16, 65536); /* up to 64 GB (FAT32, sparse image) */
}
