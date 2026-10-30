/* Requests to the serial worker, from the engine thread: open (polled until done) and close. */
#include "serial_worker_internal.h"
#include "util/util.h"

#include <stdlib.h>
#include <string.h>

void *wye_serial_worker_open(struct wye_serial_worker *w, const char *path, char *err, size_t len)
{
    if (!w) {
        wye_strcpy(err, len, "Serial worker unavailable");
        return NULL;
    }
    plat_mutex_lock(w->lock);
    plat_serial *port = NULL;
    if (w->requested && strcmp(w->path, path) != 0) {
        /* A removed/reordered device must not strand a completed open. */
        if (w->done) {
            plat_serial *unused = w->opened;
            w->requested = w->done = false;
            w->opened = NULL;
            if (unused) wye_serial_worker_close(w, unused);
        }
    }
    if (!w->requested) {
        wye_strcpy(w->path, sizeof(w->path), path);
        w->requested = true;
        plat_event_signal(w->work);
    } else if (w->done && strcmp(w->path, path) == 0) {
        port = w->opened;
        wye_strcpy(err, len, w->error);
        w->opened = NULL;
        w->requested = w->done = false;
    }
    plat_mutex_unlock(w->lock);
    return port;
}

bool wye_serial_worker_pending(struct wye_serial_worker *w)
{
    if (!w) return false;
    plat_mutex_lock(w->lock);
    bool pending = w->requested;
    plat_mutex_unlock(w->lock);
    return pending;
}

void wye_serial_worker_close(struct wye_serial_worker *w, void *port)
{
    if (!port) return;
    struct close_job *job = malloc(sizeof(*job));
    if (!job || !w) { /* allocation failure: still release the OS handle */
        free(job);
        plat_serial_close(port);
        return;
    }
    *job = (struct close_job){port, NULL};
    plat_mutex_lock(w->lock);
    *w->tail = job;
    w->tail = &job->next;
    plat_mutex_unlock(w->lock);
    plat_event_signal(w->work);
}
