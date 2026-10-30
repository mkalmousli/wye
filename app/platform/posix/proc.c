#define _DEFAULT_SOURCE
#include "proc.h"
#include "plat_sys.h"

#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

extern char **environ;

static bool spawn(const char *const argv[], pid_t *pid)
{
    return posix_spawnp(pid, argv[0], NULL, NULL, (char *const *)argv, environ) == 0;
}

bool posix_run(const char *const argv[])
{
    pid_t pid;
    int status = 0;
    if (!spawn(argv, &pid)) {
        return false;
    }
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            return false;
        }
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

/* sh backgrounds the program and exits at once, so the child is reaped here
   and the program is re-parented to init. Arguments stay positional ($0 $@). */
bool posix_spawn_detached(const char *const argv[])
{
    const char *sh[16] = {"/bin/sh", "-c", "\"$0\" \"$@\" >/dev/null 2>&1 &"};
    int n = 3;
    for (int i = 0; argv[i] && n < 15; ++i) {
        sh[n++] = argv[i];
    }
    sh[n] = NULL;
    return posix_run(sh);
}

bool plat_http_download(const char *url, const char *dest, size_t max_bytes, char *err, size_t errlen)
{
    char max[32];
    snprintf(max, sizeof(max), "%zu", max_bytes);
    /* -s without -S: failures (e.g. 404 before the first release) are reported by the caller */
    const char *argv[] = {"curl", "-fsL", "--proto", "=https", "--retry", "2",
                          "--connect-timeout", "15", "--max-time", "600",
                          "--max-filesize", max, "-o", dest, url, NULL};
    if (!posix_run(argv)) {
        snprintf(err, errlen, "download failed (is curl installed and the network up?)");
        return false;
    }
    return true;
}
