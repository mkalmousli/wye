/* Threads, locks, events and time. Implemented per OS. */
#ifndef WYE_PLAT_THREAD_H
#define WYE_PLAT_THREAD_H

#include <stdbool.h>
#include <stdint.h>

typedef struct plat_thread plat_thread;
typedef struct plat_mutex plat_mutex;
typedef struct plat_event plat_event;

plat_thread *plat_thread_start(void (*fn)(void *), void *arg);
void plat_thread_join(plat_thread *t);

plat_mutex *plat_mutex_new(void);
void plat_mutex_lock(plat_mutex *m);
void plat_mutex_unlock(plat_mutex *m);
void plat_mutex_free(plat_mutex *m);

/* Auto-reset event: wait returns true if signalled, false on timeout. */
plat_event *plat_event_new(void);
void plat_event_signal(plat_event *e);
bool plat_event_wait(plat_event *e, int timeout_ms);
void plat_event_free(plat_event *e);

uint64_t plat_now_ms(void);     /* monotonic */
int64_t plat_unix_time(void);   /* wall clock, seconds */
void plat_sleep_ms(int ms);
/* Request 1 ms scheduler granularity (Windows); no-op elsewhere. */
void plat_timer_precision(bool high);

#endif
