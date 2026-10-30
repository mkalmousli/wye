/*
 * The engine owns the device link, input routing and all features, runs
 * them on its own thread, and exposes a thread-safe API to the UI.
 */
#ifndef WYE_ENGINE_H
#define WYE_ENGINE_H

#include "activity.h"
#include "audio/audio.h"
#include "config/config.h"
#include "drive/drive.h"
#include "fw/fw_session.h"
#include "input/input.h"
#include "link/link.h"
#include "plat_input.h"

#define WYE_NOTICE_LEN 160

struct wye_api_ops;

/* One-shot requests the UI must service on its own thread. */
enum wye_ui_request {
    WYE_UI_REQ_NONE = 0,
    WYE_UI_REQ_PASTE = 1u << 0,     /* read clipboard, call wye_engine_type_text */
    WYE_UI_REQ_SHOW = 1u << 1,      /* bring the main window to front */
    WYE_UI_REQ_MINI = 1u << 2,      /* switch between mini panel and full window */
    WYE_UI_REQ_FOLDER = 1u << 3,    /* open the shared folder */
};

struct wye_status {
    struct wye_link_info link;
    struct wye_activity activity;   /* motion counters restart with each wye_engine_status() */
    enum wye_fwd_mode mode;
    enum plat_input_status capture;
    bool jiggler_on;
    int jiggler_progress;
    bool recording;
    unsigned long recorded_events;
    bool playing, play_paused, play_loop;
    int play_progress, play_speed;
    char play_name[96];
    bool typing;
    enum wye_fw_phase fw_phase;
    uint8_t fw_node;
    int fw_progress;
    char fw_error[128];
    bool bridge_outdated, target_outdated;
    bool learning_key;
    int learned_key;
    bool identity_valid[3];         /* indexed by node id */
    struct wye_identity identity[3];
    struct wye_drive_status drive;
    struct wye_audio_status audio;  /* sound routes between the bridge and this computer */
    bool functions_valid;           /* target answered WYE_MSG_FUNCS */
    struct wye_functions functions;                /* HID usage, 0 until a key was pressed */
};

bool wye_engine_start(const struct wye_config *cfg);
void wye_engine_stop(void);
void wye_engine_status(struct wye_status *out);
void wye_engine_apply_config(const struct wye_config *cfg);

void wye_engine_set_mode(enum wye_fwd_mode mode);
void wye_engine_toggle(void);
void wye_engine_type_text(const char *text);
void wye_engine_ctrl_alt_del(void);
void wye_engine_jiggler(bool on);
/* Hotkey learning: the next key press is captured (not delivered anywhere). */
void wye_engine_learn_key(bool start);

bool wye_engine_record(bool start, const char *name);
bool wye_engine_play(const char *path, bool loop, char *err, size_t errlen);
void wye_engine_play_stop(void);
void wye_engine_play_pause(void);
void wye_engine_play_speed(int delta_percent);

bool wye_engine_update_firmware(uint8_t role);
/* Shared folder: look for changes on both sides right away (see drive/drive.h). */
void wye_engine_drive_sync_now(void);
bool wye_engine_reboot_node(uint8_t node, uint8_t mode);
/* Runtime USB identity of a node (applied by the device on reboot). */
bool wye_engine_identity_request(uint8_t node);
bool wye_engine_identity_set(uint8_t node, const struct wye_identity *id);
/* The target's USB parts (names, drive on/off); applied when it restarts. */
bool wye_engine_functions_request(void);
bool wye_engine_functions_set(const struct wye_functions *f);
/* Legacy/old firmware: reboot a generic Pico on `port` into BOOTSEL. */
bool wye_engine_rescue_legacy(void);

/* The local API's view of the engine (engine_remote.c). */
const struct wye_api_ops *wye_engine_api_ops(void);

unsigned wye_engine_take_ui_requests(void);
/* Pops the oldest user-facing notice; false if none. */
bool wye_engine_take_notice(char *out, size_t len);
void wye_engine_notice(const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

#endif
