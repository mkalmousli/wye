#include "log.h"

#include "plat_sys.h"
#include "plat_thread.h"
#include "util/util.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define LOG_FILE_MAX (1024L * 1024L)

static plat_mutex *g_lock;
static FILE *g_file;
static char g_path[PLAT_PATH_MAX];
static char g_ring[WYE_LOG_RING][WYE_LOG_LINE];
static int g_head, g_count;

static void rotate_if_needed(void)
{
    if (!g_file || ftell(g_file) < LOG_FILE_MAX) {
        return;
    }
    char old[PLAT_PATH_MAX];
    fclose(g_file);
    wye_strcpy(old, sizeof(old), g_path);
    wye_strcat(old, sizeof(old), ".1");
    plat_rename_replace(g_path, old);
    g_file = fopen(g_path, "a");
}

void wye_log_init(const char *dir)
{
    if (!g_lock) {
        g_lock = plat_mutex_new();
    }
    if (dir && wye_path_join(g_path, sizeof(g_path), dir, "wye.log")) {
        g_file = fopen(g_path, "a");
    }
}

void wye_log_shutdown(void)
{
    if (g_file) {
        fclose(g_file);
        g_file = NULL;
    }
}

void wye_log(enum wye_log_level level, const char *fmt, ...)
{
    static const char *names[] = {"debug", "info", "warn", "error"};
    char msg[WYE_LOG_LINE - 32];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    if (g_lock) {
        plat_mutex_lock(g_lock);
    }
    time_t now = time(NULL);
    struct tm tm_now = *localtime(&now); /* localtime is not reentrant: keep under lock */
    char *line = g_ring[(g_head + g_count) % WYE_LOG_RING];
    snprintf(line, WYE_LOG_LINE, "%02d:%02d:%02d %-5s %s", tm_now.tm_hour, tm_now.tm_min,
             tm_now.tm_sec, names[level], msg);
    if (g_count < WYE_LOG_RING) {
        g_count++;
    } else {
        g_head = (g_head + 1) % WYE_LOG_RING;
    }
    if (g_file) {
        fprintf(g_file, "%04d-%02d-%02d %s\n", tm_now.tm_year + 1900, tm_now.tm_mon + 1,
                tm_now.tm_mday, line);
        fflush(g_file);
        rotate_if_needed();
    }
    if (g_lock) {
        plat_mutex_unlock(g_lock);
    }
}

int wye_log_snapshot(char (*lines)[WYE_LOG_LINE], int max)
{
    plat_mutex_lock(g_lock);
    int n = g_count < max ? g_count : max;
    int start = g_count - n;
    for (int i = 0; i < n; ++i) {
        wye_strcpy(lines[i], WYE_LOG_LINE, g_ring[(g_head + start + i) % WYE_LOG_RING]);
    }
    plat_mutex_unlock(g_lock);
    return n;
}

const char *wye_log_path(void)
{
    return g_path;
}
