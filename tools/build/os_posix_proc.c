/* Linux/macOS primitives: processes. */
#include "bld.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

intptr_t proc_start(const strlist *argv)
{
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv->items[0], argv->items);
        fprintf(stderr, "build: cannot run %s\n", argv->items[0]);
        _exit(127);
    }
    return pid > 0 ? (intptr_t)pid : -1;
}

int proc_wait_any(const intptr_t *procs, int n, bool *ok)
{
    for (;;) {
        int status = 0;
        pid_t pid = waitpid(-1, &status, 0);
        if (pid < 0 && errno != EINTR) {
            *ok = false;
            return 0;
        }
        for (int i = 0; i < n; ++i) {
            if (procs[i] == (intptr_t)pid) {
                *ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
                return i;
            }
        }
    }
}

char *run_capture(const char *cmd)
{
    char *full = str_fmt("%s 2>/dev/null", cmd);
    FILE *p = popen(full, "r");
    free(full);
    char buf[512] = "";
    size_t n = p ? fread(buf, 1, sizeof(buf) - 1, p) : 0;
    buf[n] = '\0';
    bool ok = p && pclose(p) == 0;
    return ok ? str_fmt("%s", str_trim(buf)) : NULL;
}

bool exec_self(const char *exe, char **argv)
{
    execv(exe, argv);
    return false;
}

int cpu_count(void)
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (int)n : 1;
}
