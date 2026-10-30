#define _POSIX_C_SOURCE 200809L
#include "plat_thread.h"

#include <time.h>

uint64_t plat_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

int64_t plat_unix_time(void)
{
    return (int64_t)time(NULL);
}

void plat_sleep_ms(int ms)
{
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

void plat_timer_precision(bool high)
{
    (void)high; /* POSIX timers are already fine-grained */
}
