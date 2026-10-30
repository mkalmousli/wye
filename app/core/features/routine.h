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

struct wye_recorder {
    FILE *file;
    uint64_t start;
    unsigned long events;
    char path[PLAT_PATH_MAX];
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

#endif
