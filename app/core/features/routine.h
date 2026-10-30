/*
 * Routines: recording what is sent to the target and replaying it.
 *
 * File format (text, one event per line):
 *   # wye-routine 2
 *   <milliseconds> <message type> <payload hex>
 */
#ifndef WYE_ROUTINE_H
#define WYE_ROUTINE_H

#include "plat_sys.h"
#include "sender.h"

#include <stdio.h>

#define WYE_ROUTINE_EXT ".hwr"
#define WYE_ROUTINE_NAME_MAX 96
#define WYE_ROUTINE_LIST_MAX 256

struct wye_routine_event {
    uint32_t t;
    uint8_t type;
    uint8_t len;
    uint8_t payload[16];
};

#define WYE_ROUTINE_TAIL 128   /* newest events kept for watching a recording live */

struct wye_recorder {
    FILE *file;
    uint64_t start;
    unsigned long events;
    char path[PLAT_PATH_MAX];  /* stays set after stopping: the last recording */
    struct wye_routine_event tail[WYE_ROUTINE_TAIL];
};

struct wye_player {
    bool active, paused, loop;
    int speed_percent;
    struct wye_routine_event *events;
    size_t count, index;
    uint64_t start, paused_at, paused_total;
    char name[WYE_ROUTINE_NAME_MAX];
};

struct wye_routine_info {
    char name[WYE_ROUTINE_NAME_MAX];
    char path[PLAT_PATH_MAX];
};

bool wye_routines_dir(char *out, size_t len);
/* Turns user text into a safe file stem ("My Macro!" -> "My-Macro"). */
void wye_routine_sanitize(const char *in, char *out, size_t len);
int wye_routine_list(struct wye_routine_info *out, int max);
bool wye_routine_rename(const char *path, const char *new_name, char *err, size_t errlen);
bool wye_routine_delete(const char *path);

bool wye_recorder_start(struct wye_recorder *r, const char *name, uint64_t now);
void wye_recorder_note(struct wye_recorder *r, uint64_t now, uint8_t type, const void *p, size_t len);
void wye_recorder_stop(struct wye_recorder *r);

bool wye_player_load(struct wye_player *p, const char *path, char *err, size_t errlen);
void wye_player_start(struct wye_player *p, uint64_t now, bool loop);
void wye_player_tick(struct wye_player *p, uint64_t now, const struct wye_sender *out);
void wye_player_pause(struct wye_player *p, uint64_t now);
void wye_player_stop(struct wye_player *p);
uint64_t wye_player_elapsed(const struct wye_player *p, uint64_t now);
int wye_player_progress(const struct wye_player *p, uint64_t now);

/* Parses one line; false for comments/invalid lines. */
bool wye_routine_parse_line(const char *line, struct wye_routine_event *ev);
/* The newest recorded events, oldest first; returns how many were copied. */
int wye_recorder_tail(const struct wye_recorder *r, struct wye_routine_event *out, int max);

/* A routine opened for editing (routine_doc.c). Times are absolute and never go backwards. */
struct wye_routine_doc {
    struct wye_routine_event *ev;
    size_t count, cap;
};

struct wye_routine_compress {
    bool merge_moves;       /* join mouse moves closer together than merge_ms into one */
    uint32_t merge_ms;
    bool cap_pauses;        /* no wait longer than max_pause_ms, nothing before the first event */
    uint32_t max_pause_ms;
};

#define WYE_ROUTINE_MERGE_MS 30u
#define WYE_ROUTINE_MAX_PAUSE_MS 1000u

bool wye_routine_doc_load(struct wye_routine_doc *d, const char *path, char *err, size_t errlen);
bool wye_routine_doc_save(const struct wye_routine_doc *d, const char *path);
void wye_routine_doc_free(struct wye_routine_doc *d);
/* Removes from..to (inclusive); what followed moves up so no empty wait is left. */
void wye_routine_doc_delete(struct wye_routine_doc *d, size_t from, size_t to);
/* Inserts a copy of from..to right after it. */
bool wye_routine_doc_duplicate(struct wye_routine_doc *d, size_t from, size_t to);
/* Swaps event i with the next or previous one (dir -1 or 1); their times stay in place. */
bool wye_routine_doc_move(struct wye_routine_doc *d, size_t i, int dir);
uint32_t wye_routine_doc_delay(const struct wye_routine_doc *d, size_t i);  /* the wait before event i */
void wye_routine_doc_set_delay(struct wye_routine_doc *d, size_t i, uint32_t ms);
void wye_routine_doc_speed(struct wye_routine_doc *d, int percent);         /* 200: twice as fast, for good */
/* Removes what does nothing (repeated key states, empty mouse reports), and optionally more;
   key presses and button changes are never dropped. Returns events removed. */
size_t wye_routine_doc_compress(struct wye_routine_doc *d, const struct wye_routine_compress *o);
bool wye_routine_is_move(const struct wye_routine_event *ev);   /* mouse motion and nothing else */

#endif
