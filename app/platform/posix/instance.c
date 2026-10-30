/*
 * Single instance via an advisory lock file holding the PID. A second launch
 * sends SIGUSR1 to the running instance, which then shows its window.
 */
#define _DEFAULT_SOURCE
#include "plat_sys.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>

static volatile sig_atomic_t g_show_requested;

static void on_usr1(int sig)
{
    (void)sig;
    g_show_requested = 1;
}

bool plat_instance_acquire(bool notify)
{
    char dir[PLAT_PATH_MAX], path[PLAT_PATH_MAX + 32];
    const char *rt = getenv("XDG_RUNTIME_DIR");
    if (rt && *rt) {
        snprintf(dir, sizeof(dir), "%s", rt);
    } else if (!plat_temp_dir(dir, sizeof(dir))) {
        return true; /* cannot tell; do not block startup */
    }
    snprintf(path, sizeof(path), "%s/wye.lock", dir);
    int fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (fd < 0) {
        return true;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        char buf[32] = "";
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        pid_t other = n > 0 ? (pid_t)atoi(buf) : 0;
        if (notify && other > 1) {
            kill(other, SIGUSR1);
        }
        close(fd);
        return false;
    }
    char pid[32];
    int len = snprintf(pid, sizeof(pid), "%d\n", (int)getpid());
    if (ftruncate(fd, 0) == 0 && write(fd, pid, (size_t)len) == len) {
        /* fd stays open (and locked) for the life of the process */
    }
    signal(SIGUSR1, on_usr1);
    signal(SIGPIPE, SIG_IGN);
    return true;
}

bool plat_instance_show_requested(void)
{
    bool r = g_show_requested != 0;
    g_show_requested = 0;
    return r;
}
