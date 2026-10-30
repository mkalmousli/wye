/* Running commands, one at a time or in parallel. */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_PARALLEL 64 /* WaitForMultipleObjects limit */

void echo_cmd(const char *tag, const strlist *argv, const char *label)
{
    if (B.verbose || !label) {
        for (size_t i = 0; i < argv->count; ++i) {
            printf("%s%s", i ? " " : "", argv->items[i]);
        }
        printf("\n");
    } else {
        printf("  %-5s %s\n", tag, label);
    }
}

bool run(const strlist *argv)
{
    intptr_t p = proc_start(argv);
    bool ok = false;
    if (p >= 0) {
        proc_wait_any(&p, 1, &ok);
    }
    return ok;
}

/* Runs up to B.jobs commands at once; stops starting new ones after a failure. */
bool jobs_run(const strlist *cmds, size_t n, char *const *labels, const char *tag)
{
    intptr_t running[MAX_PARALLEL];
    int active = 0;
    int limit = B.jobs < 1 ? 1 : (B.jobs > MAX_PARALLEL ? MAX_PARALLEL : B.jobs);
    bool all_ok = true;
    size_t next = 0;
    while (active > 0 || (next < n && all_ok)) {
        if (next < n && all_ok && active < limit) {
            echo_cmd(tag, &cmds[next], labels[next]);
            intptr_t p = proc_start(&cmds[next++]);
            if (p < 0) {
                all_ok = false;
            } else {
                running[active++] = p;
            }
            continue;
        }
        bool ok = false;
        int i = proc_wait_any(running, active, &ok);
        running[i] = running[--active];
        all_ok = all_ok && ok;
    }
    return all_ok;
}
