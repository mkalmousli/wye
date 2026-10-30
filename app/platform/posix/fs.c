#define _DEFAULT_SOURCE
#include "plat_sys.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void plat_cleanup_previous_version(void)
{
    /* Nothing to do: POSIX replaces the binary in place with rename(). */
}

const char *plat_path_sep(void)
{
    return "/";
}

bool plat_path_exists(const char *path)
{
    return access(path, F_OK) == 0;
}

bool plat_rename_replace(const char *from, const char *to)
{
    return rename(from, to) == 0;
}

bool plat_mkdirs(const char *path)
{
    char buf[PLAT_PATH_MAX];
    size_t n = strlen(path);
    if (n == 0 || n >= sizeof(buf)) {
        return false;
    }
    memcpy(buf, path, n + 1);
    for (char *p = buf + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
                return false;
            }
            *p = '/';
        }
    }
    return mkdir(buf, 0755) == 0 || errno == EEXIST;
}

bool plat_temp_dir(char *out, size_t len)
{
    const char *t = getenv("TMPDIR");
    int n = snprintf(out, len, "%s/wye-%d", t && *t ? t : "/tmp", (int)getuid());
    return n > 0 && (size_t)n < len && plat_mkdirs(out);
}

bool plat_file_sparse(FILE *f)
{
    (void)f; /* writing past the end leaves holes on POSIX filesystems */
    return true;
}
