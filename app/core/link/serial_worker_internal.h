/* Shared by serial_worker.c (the thread) and serial_worker_jobs.c (its requests). */
#ifndef WYE_SERIAL_WORKER_INTERNAL_H
#define WYE_SERIAL_WORKER_INTERNAL_H

#include "serial_worker.h"
#include "plat_thread.h"

struct close_job { plat_serial *port; struct close_job *next; };
struct wye_serial_worker {
    plat_mutex *lock;
    plat_event *work, *wake;
    plat_thread *thread;
    struct wye_serial_ops ops;
    struct close_job *closing, **tail;
    bool stop, requested, done;
    char path[PLAT_PORT_PATH_MAX], error[128];
    plat_serial *opened;
};

#endif
