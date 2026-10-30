/* User-facing notices (shown as toasts by the UI). */
#include "engine_internal.h"

#include "log.h"
#include "util/util.h"

#include <stdarg.h>
#include <stdio.h>

void wye_engine_notice_locked(struct wye_engine *e, const char *msg)
{
    LOG_I("notice: %s", msg);
    int slot = (e->notice_head + e->notice_count) % WYE_NOTICES;
    if (e->notice_count == WYE_NOTICES) {
        e->notice_head = (e->notice_head + 1) % WYE_NOTICES;
    } else {
        e->notice_count++;
    }
    wye_strcpy(e->notices[slot], WYE_NOTICE_LEN, msg);
}

void wye_engine_notice(const char *fmt, ...)
{
    char msg[WYE_NOTICE_LEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    plat_mutex_lock(g_engine.lock);
    wye_engine_notice_locked(&g_engine, msg);
    plat_mutex_unlock(g_engine.lock);
}

bool wye_engine_take_notice(char *out, size_t len)
{
    struct wye_engine *e = &g_engine;
    plat_mutex_lock(e->lock);
    bool have = e->notice_count > 0;
    if (have) {
        wye_strcpy(out, len, e->notices[e->notice_head]);
        e->notice_head = (e->notice_head + 1) % WYE_NOTICES;
        e->notice_count--;
    }
    plat_mutex_unlock(e->lock);
    return have;
}
