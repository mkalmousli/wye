#define _GNU_SOURCE
#include "plat_thread.h"
#include "posix_event.h"

#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

struct plat_thread { pthread_t t; void (*fn)(void *); void *arg; };
struct plat_mutex { pthread_mutex_t m; };
struct plat_event { int fds[2]; };

static void *trampoline(void *p)
{
    struct plat_thread *t = p;
    t->fn(t->arg);
    return NULL;
}

plat_thread *plat_thread_start(void (*fn)(void *), void *arg)
{
    struct plat_thread *t = calloc(1, sizeof(*t));
    if (!t) {
        return NULL;
    }
    t->fn = fn;
    t->arg = arg;
    if (pthread_create(&t->t, NULL, trampoline, t) != 0) {
        free(t);
        return NULL;
    }
    return t;
}

void plat_thread_join(plat_thread *t)
{
    if (t) {
        pthread_join(t->t, NULL);
        free(t);
    }
}

plat_mutex *plat_mutex_new(void)
{
    struct plat_mutex *m = calloc(1, sizeof(*m));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m->m, &attr);
    pthread_mutexattr_destroy(&attr);
    return m;
}

void plat_mutex_lock(plat_mutex *m) { pthread_mutex_lock(&m->m); }
void plat_mutex_unlock(plat_mutex *m) { pthread_mutex_unlock(&m->m); }
void plat_mutex_free(plat_mutex *m) { pthread_mutex_destroy(&m->m); free(m); }

plat_event *plat_event_new(void)
{
    struct plat_event *e = calloc(1, sizeof(*e));
    if (!e || pipe(e->fds) != 0) {
        free(e);
        return NULL;
    }
    for (int i = 0; i < 2; ++i) {
        fcntl(e->fds[i], F_SETFL, fcntl(e->fds[i], F_GETFL) | O_NONBLOCK);
        fcntl(e->fds[i], F_SETFD, FD_CLOEXEC);
    }
    return e;
}

void plat_event_signal(plat_event *e)
{
    char c = 1;
    ssize_t r = write(e->fds[1], &c, 1); /* full pipe = already signalled */
    (void)r;
}

int plat_event_fd(plat_event *e) { return e->fds[0]; }

void plat_event_drain(plat_event *e)
{
    char buf[64];
    while (read(e->fds[0], buf, sizeof(buf)) > 0) {
    }
}

bool plat_event_wait(plat_event *e, int timeout_ms)
{
    struct pollfd p = {e->fds[0], POLLIN, 0};
    bool hit = poll(&p, 1, timeout_ms) > 0;
    plat_event_drain(e);
    return hit;
}

void plat_event_free(plat_event *e)
{
    close(e->fds[0]);
    close(e->fds[1]);
    free(e);
}
