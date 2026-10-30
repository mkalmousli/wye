/* Recording routines and parsing their lines. */
#include "routine.h"

#include "wye_proto.h"
#include "log.h"
#include "util/util.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

bool wye_recorder_start(struct wye_recorder *r, const char *name, uint64_t now)
{
    char dir[PLAT_PATH_MAX], stem[WYE_ROUTINE_NAME_MAX], file[WYE_ROUTINE_NAME_MAX + 64];
    if (r->file || !wye_routines_dir(dir, sizeof(dir))) {
        return false;
    }
    wye_routine_sanitize(name, stem, sizeof(stem));
    time_t t = time(NULL);
    struct tm tm_now = *localtime(&t);
    snprintf(file, sizeof(file), "%s-%04d%02d%02d-%02d%02d%02d" WYE_ROUTINE_EXT,
             stem[0] ? stem : "routine", tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
             tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
    if (!wye_path_join(r->path, sizeof(r->path), dir, file) || !(r->file = fopen(r->path, "w"))) {
        LOG_E("routine: cannot create %s", r->path);
        return false;
    }
    fprintf(r->file, "# wye-routine 2\n");
    r->start = now;
    r->events = 0;
    LOG_I("routine: recording to %s", r->path);
    return true;
}

void wye_recorder_note(struct wye_recorder *r, uint64_t now, uint8_t type, const void *p, size_t len)
{
    char hex[2 * 16 + 1];
    bool input = type >= WYE_MSG_KEYBOARD && type <= WYE_MSG_ABS_MOUSE;
    if (!r->file || !input || len > 16) {
        return;
    }
    wye_hex_encode(p, len, hex, sizeof(hex));
    fprintf(r->file, "%lu %u %s\n", (unsigned long)(now - r->start), type, hex);
    struct wye_routine_event *t = &r->tail[r->events % WYE_ROUTINE_TAIL];
    t->t = (uint32_t)(now - r->start);
    t->type = type;
    t->len = (uint8_t)len;
    memcpy(t->payload, p, len);
    r->events++;
}

int wye_recorder_tail(const struct wye_recorder *r, struct wye_routine_event *out, int max)
{
    unsigned long have = r->events < WYE_ROUTINE_TAIL ? r->events : WYE_ROUTINE_TAIL;
    int n = have < (unsigned long)max ? (int)have : max;
    for (int i = 0; i < n; ++i) {
        out[i] = r->tail[(r->events - (unsigned long)n + (unsigned long)i) % WYE_ROUTINE_TAIL];
    }
    return n;
}

void wye_recorder_stop(struct wye_recorder *r)
{
    if (r->file) {
        fclose(r->file);
        r->file = NULL;
        LOG_I("routine: saved %lu events", r->events);
    }
}

bool wye_routine_parse_line(const char *line, struct wye_routine_event *ev)
{
    char *end;
    if (*line == '#' || *line == '\0') {
        return false;
    }
    unsigned long t = strtoul(line, &end, 10);
    unsigned long type = strtoul(end, &end, 10);
    while (*end == ' ') {
        end++;
    }
    char hex[2 * 16 + 1];
    size_t n = strcspn(end, " \r\n");
    if (end == line || type > 255 || n > 32 || n % 2) {
        return false;
    }
    memcpy(hex, end, n);
    hex[n] = '\0';
    size_t len = wye_hex_decode(hex, ev->payload, sizeof(ev->payload));
    if (len == (size_t)-1) {
        return false;
    }
    ev->t = (uint32_t)t;
    ev->type = (uint8_t)type;
    ev->len = (uint8_t)len;
    return true;
}
