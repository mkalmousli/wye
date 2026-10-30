/*
 * Thread-safe logging to a rotating file in the data directory and to an
 * in-memory ring the UI shows under Diagnostics.
 */
#ifndef WYE_LOG_H
#define WYE_LOG_H

#include <stddef.h>

enum wye_log_level { WYE_LOG_DEBUG = 0, WYE_LOG_INFO, WYE_LOG_WARN, WYE_LOG_ERROR };

#define WYE_LOG_RING 256
#define WYE_LOG_LINE 192

void wye_log_init(const char *dir);
void wye_log_shutdown(void);
void wye_log(enum wye_log_level level, const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

/* Copies up to max most recent lines (oldest first); returns count. */
int wye_log_snapshot(char (*lines)[WYE_LOG_LINE], int max);
/* Full path of the current log file (for "Open log folder"). */
const char *wye_log_path(void);

#define LOG_D(...) wye_log(WYE_LOG_DEBUG, __VA_ARGS__)
#define LOG_I(...) wye_log(WYE_LOG_INFO, __VA_ARGS__)
#define LOG_W(...) wye_log(WYE_LOG_WARN, __VA_ARGS__)
#define LOG_E(...) wye_log(WYE_LOG_ERROR, __VA_ARGS__)

#endif
