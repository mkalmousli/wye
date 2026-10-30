/* Serial USB control requests can take seconds on a faulty device. Keep port
   open/close operations off the engine and UI threads. */
#ifndef WYE_SERIAL_WORKER_H
#define WYE_SERIAL_WORKER_H

#include "plat_serial.h"

struct wye_serial_worker;
struct wye_serial_ops {
    plat_serial *(*open)(const char *path, char *err, size_t len);
    void (*close)(plat_serial *port);
};

struct wye_serial_worker *wye_serial_worker_new(plat_event *wake, const struct wye_serial_ops *ops);
void *wye_serial_worker_open(struct wye_serial_worker *w, const char *path, char *err, size_t len);
bool wye_serial_worker_pending(struct wye_serial_worker *w);
void wye_serial_worker_close(struct wye_serial_worker *w, void *port);
void wye_serial_worker_free(struct wye_serial_worker *w);

#endif
