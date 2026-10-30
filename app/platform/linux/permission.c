/*
 * Input access on Linux: /dev/input/event* (read + grab) and /dev/uinput.
 * Granted once through a graphical pkexec prompt that runs this binary
 * with --install-permissions (see permission_install.c).
 */
#define _DEFAULT_SOURCE
#include "plat_sys.h"
#include "proc.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static bool any_event_readable(void)
{
    DIR *d = opendir("/dev/input");
    bool ok = false;
    for (struct dirent *e = d ? readdir(d) : NULL; e && !ok; e = readdir(d)) {
        if (strncmp(e->d_name, "event", 5) == 0) {
            char path[300];
            snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
            ok = access(path, R_OK) == 0;
        }
    }
    if (d) {
        closedir(d);
    }
    return ok;
}

enum plat_permission plat_input_permission(void)
{
    bool uinput = access("/dev/uinput", W_OK) == 0;
    return uinput && any_event_readable() ? PLAT_PERM_OK : PLAT_PERM_NEEDED;
}

bool plat_input_permission_request(char *err, size_t errlen)
{
    char exe[PLAT_PATH_MAX];
    if (!plat_exe_path(exe, sizeof(exe))) {
        snprintf(err, errlen, "cannot locate the running program");
        return false;
    }
    const char *argv[] = {"pkexec", exe, "--install-permissions", NULL};
    if (!posix_run(argv)) {
        snprintf(err, errlen, "permission setup was cancelled or pkexec is not available");
        return false;
    }
    for (int i = 0; i < 20 && plat_input_permission() != PLAT_PERM_OK; ++i) {
        usleep(100 * 1000); /* udev applies ACLs asynchronously */
    }
    return plat_input_permission() == PLAT_PERM_OK;
}
