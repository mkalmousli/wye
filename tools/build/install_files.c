/* File helpers for ./build.c install and uninstall. */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <sys/stat.h>
#endif

/* $HOME (or %USERPROFILE%) joined with `sub`; exits when it is unset. */
char *inst_home(const char *sub)
{
    const char *home = getenv("HOME") ? getenv("HOME") : getenv("USERPROFILE");
    if (!home || !*home) {
        fprintf(stderr, "build: HOME is not set; pass --prefix=DIR\n");
        exit(1);
    }
    return str_fmt("%s/%s", home, sub);
}

/* Copies via a temporary file and a rename, so a running copy can be replaced. */
bool inst_copy(const char *from, const char *to, bool executable)
{
    size_t len = 0;
    char *data = fs_read(from, &len), *tmp = str_fmt("%s.new", to);
    FILE *f = data && fs_mkdir_parent(to) ? fopen(tmp, "wb") : NULL;
    bool ok = f && fwrite(data, 1, len, f) == len;
    ok = f && fclose(f) == 0 && ok;
#if !defined(_WIN32)
    ok = ok && chmod(tmp, executable ? 0755 : 0644) == 0;
#else
    (void)executable;
#endif
    ok = ok && fs_rename(tmp, to);
    if (!ok) {
        remove(tmp);
        fprintf(stderr, "build: cannot install %s (%s)\n", to, data ? "no write access?" : "missing source");
    } else {
        printf("  INST  %s\n", to);
    }
    free(data);
    free(tmp);
    return ok;
}

bool inst_write(const char *path, const char *text)
{
    bool ok = fs_write_if_changed(path, text, strlen(text));
    printf("  %s  %s\n", ok ? "INST" : "FAIL", path);
    return ok;
}

void inst_remove(const char *path)
{
    if (fs_exists(path)) {
        bool ok = remove(path) == 0;
        printf("  %s  %s\n", ok ? "RM  " : "FAIL", path);
    }
}

/* Runs an optional helper (cache refreshers and the like); failures are ignored. */
void inst_optional(const char *cmd)
{
    free(run_capture(cmd));
}
