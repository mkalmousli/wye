/* Running helper programs without a shell (no quoting/injection issues). */
#ifndef WYE_POSIX_PROC_H
#define WYE_POSIX_PROC_H

#include <stdbool.h>

/* Runs argv (NULL-terminated), waits, returns true on exit status 0. */
bool posix_run(const char *const argv[]);
/* Starts argv detached and returns immediately. */
bool posix_spawn_detached(const char *const argv[]);

#endif
