/* POSIX event internals: a self-pipe, so serial reads can poll() on it. */
#ifndef WYE_POSIX_EVENT_H
#define WYE_POSIX_EVENT_H

#include "plat_thread.h"

int plat_event_fd(plat_event *e);   /* read end, for poll() */
void plat_event_drain(plat_event *e);

#endif
