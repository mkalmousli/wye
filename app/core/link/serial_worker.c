/* The worker thread: closes queued ports first, then runs the one requested open. */
#include "serial_worker_internal.h"
#include "util/util.h"

#include <stdlib.h>
#include <string.h>

static void service(void *arg)
{
    struct wye_serial_worker *w = arg;
    for (;;) {
        plat_mutex_lock(w->lock);
        struct close_job *job = w->closing;
        if (job) {
            w->closing = job->next;
            if (!w->closing) w->tail = &w->closing;
        }
        bool open = !job && w->requested && !w->done;
        bool stop = w->stop && !job && !open;
        char path[PLAT_PORT_PATH_MAX];
        wye_strcpy(path, sizeof(path), w->path);
        plat_mutex_unlock(w->lock);
        if (stop) break;
        if (job) {
            w->ops.close(job->port);
            free(job);
        } else if (open) {
            char err[128] = "";
            plat_serial *port = w->ops.open(path, err, sizeof(err));
            plat_mutex_lock(w->lock);
            w->opened = port;
            wye_strcpy(w->error, sizeof(w->error), err);
            w->done = true;
            plat_mutex_unlock(w->lock);
            plat_event_signal(w->wake);
        } else {
            plat_event_wait(w->work, 1000);
        }
    }
}

struct wye_serial_worker *wye_serial_worker_new(plat_event *wake, const struct wye_serial_ops *ops)
{
    struct wye_serial_worker *w = calloc(1, sizeof(*w));
    if (!w) return NULL;
    w->lock = plat_mutex_new();
    w->work = plat_event_new();
    w->wake = wake;
    w->ops = *ops;
    w->tail = &w->closing;
    if (w->lock && w->work) w->thread = plat_thread_start(service, w);
    if (!w->thread) {
        if (w->lock) plat_mutex_free(w->lock);
        if (w->work) plat_event_free(w->work);
        free(w);
        return NULL;
    }
    return w;
}

void wye_serial_worker_free(struct wye_serial_worker *w)
{
    if (!w) return;
    plat_mutex_lock(w->lock);
    w->stop = true;
    plat_mutex_unlock(w->lock);
    plat_event_signal(w->work);
    plat_thread_join(w->thread);
    if (w->opened) w->ops.close(w->opened);
    plat_event_free(w->work);
    plat_mutex_free(w->lock);
    free(w);
}
