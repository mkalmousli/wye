/* Threads, locks, events and time on Windows. */
#include "plat_thread.h"
#include "win32_event.h"

#include <mmsystem.h>
#include <stdlib.h>
#include <time.h>

struct plat_thread { HANDLE h; void (*fn)(void *); void *arg; };
struct plat_mutex { CRITICAL_SECTION cs; };   /* recursive, like the POSIX build */
struct plat_event { HANDLE h; };

static DWORD WINAPI trampoline(LPVOID p)
{
    struct plat_thread *t = p;
    t->fn(t->arg);
    return 0;
}

plat_thread *plat_thread_start(void (*fn)(void *), void *arg)
{
    struct plat_thread *t = calloc(1, sizeof(*t));
    if (!t) {
        return NULL;
    }
    t->fn = fn;
    t->arg = arg;
    t->h = CreateThread(NULL, 0, trampoline, t, 0, NULL);
    if (!t->h) {
        free(t);
        return NULL;
    }
    return t;
}

void plat_thread_join(plat_thread *t)
{
    if (t) {
        WaitForSingleObject(t->h, INFINITE);
        CloseHandle(t->h);
        free(t);
    }
}

plat_mutex *plat_mutex_new(void)
{
    struct plat_mutex *m = calloc(1, sizeof(*m));
    InitializeCriticalSection(&m->cs);
    return m;
}

void plat_mutex_lock(plat_mutex *m) { EnterCriticalSection(&m->cs); }
void plat_mutex_unlock(plat_mutex *m) { LeaveCriticalSection(&m->cs); }
void plat_mutex_free(plat_mutex *m) { DeleteCriticalSection(&m->cs); free(m); }

plat_event *plat_event_new(void)
{
    struct plat_event *e = calloc(1, sizeof(*e));
    e->h = CreateEventA(NULL, FALSE, FALSE, NULL); /* auto-reset */
    return e;
}

void plat_event_signal(plat_event *e) { SetEvent(e->h); }
bool plat_event_wait(plat_event *e, int ms) { return WaitForSingleObject(e->h, (DWORD)ms) == WAIT_OBJECT_0; }
void plat_event_free(plat_event *e) { CloseHandle(e->h); free(e); }
HANDLE plat_event_handle(plat_event *e) { return e->h; }

uint64_t plat_now_ms(void)
{
    return GetTickCount64();
}

int64_t plat_unix_time(void)
{
    return (int64_t)time(NULL);
}

void plat_sleep_ms(int ms)
{
    Sleep((DWORD)ms);
}

void plat_timer_precision(bool high)
{
    static bool active;
    if (high != active) {
        active = high;
        if (high) {
            timeBeginPeriod(1); /* 1 ms wakeups while controlling the target */
        } else {
            timeEndPeriod(1);
        }
    }
}
