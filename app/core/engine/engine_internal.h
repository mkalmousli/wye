/* Engine state shared between the engine_*.c units. */
#ifndef WYE_ENGINE_INTERNAL_H
#define WYE_ENGINE_INTERNAL_H

#include "engine.h"
#include "drive/drive.h"
#include "features/jiggler.h"
#include "features/routine.h"
#include "features/typing.h"
#include "plat_thread.h"
#include "link/serial_worker.h"

#define WYE_NOTICES 8

struct wye_engine {
    plat_mutex *lock;
    plat_event *wake;
    plat_thread *thread;
    volatile bool stop;

    struct wye_config cfg;
    struct wye_link link;
    struct wye_link_transport transport;
    struct wye_serial_worker *serial_worker;
    struct wye_input input;
    struct wye_sender sender;      /* -> link, also feeds the recorder */
    struct wye_jiggler jiggler;
    struct wye_typing typing;
    struct wye_recorder recorder;
    struct wye_player player;
    struct wye_fw_session fw;
    struct wye_drive drive;
    struct wye_activity activity;
    bool auto_fw_tried[3];         /* per node id, once per connection */
    uint64_t next_capture_retry;
    bool learning_key;
    int learned_key;
    bool identity_valid[3];
    struct wye_identity identity[3];
    bool functions_valid;
    struct wye_functions functions;

    unsigned ui_requests;
    char notices[WYE_NOTICES][WYE_NOTICE_LEN];
    int notice_head, notice_count;
};

extern struct wye_engine g_engine;

/* engine_transport.c */
void wye_engine_transport_init(struct wye_engine *e);
/* engine_capture.c */
void wye_engine_capture_start(struct wye_engine *e);
void wye_engine_capture_watch(struct wye_engine *e, uint64_t now);
/* engine_input.c */
void wye_engine_input_init(struct wye_engine *e);
bool wye_engine_sink(void *ctx, const struct wye_input_event *ev);
void wye_engine_do_toggle(struct wye_engine *e);
void wye_engine_note_activity(struct wye_engine *e, const struct wye_input_event *ev);
/* engine_features.c */
void wye_engine_tick(struct wye_engine *e, uint64_t now);
void wye_engine_on_packet(void *ctx, const struct wye_packet *pkt);
void wye_engine_on_state(void *ctx, enum wye_link_state old_state, enum wye_link_state new_state);
/* Notices without taking the lock (caller holds it). */
void wye_engine_notice_locked(struct wye_engine *e, const char *msg);

#endif
