/* Self-update: replace the AppImage/binary in place and relaunch. */
#define _DEFAULT_SOURCE
#include "plat_sys.h"
#include "proc.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool copy_file(const char *from, const char *to)
{
    FILE *in = fopen(from, "rb");
    FILE *out = in ? fopen(to, "wb") : NULL;
    char buf[65536];
    size_t n;
    bool ok = in && out;
    while (ok && (n = fread(buf, 1, sizeof(buf), in)) > 0) {
        ok = fwrite(buf, 1, n, out) == n;
    }
    if (in) {
        fclose(in);
    }
    if (out) {
        ok = fclose(out) == 0 && ok;
    }
    return ok;
}

bool plat_install_update(const char *downloaded, char *err, size_t errlen)
{
    char exe[PLAT_PATH_MAX], staged[PLAT_PATH_MAX + 8];
    if (!plat_exe_path(exe, sizeof(exe))) {
        snprintf(err, errlen, "cannot locate the running program");
        return false;
    }
    snprintf(staged, sizeof(staged), "%s.new", exe);
    /* Stage next to the target so the final rename is atomic (same filesystem). */
    if (!copy_file(downloaded, staged)) {
        snprintf(err, errlen, "%s is not writable; this copy is managed by your package "
                 "manager - update it there", exe);
        remove(staged);
        return false;
    }
    chmod(staged, 0755);
    if (rename(staged, exe) != 0) {
        snprintf(err, errlen, "replacing %s failed: %s", exe, strerror(errno));
        remove(staged);
        return false;
    }
    const char *argv[] = {exe, "--updated", NULL};
    return posix_spawn_detached(argv);
}
