/* Linux/macOS primitives: files and directories. */
#include "bld.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

long long fs_mtime(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
#if defined(__APPLE__)
    return (long long)st.st_mtimespec.tv_sec * 1000000000LL + st.st_mtimespec.tv_nsec;
#else
    return (long long)st.st_mtim.tv_sec * 1000000000LL + st.st_mtim.tv_nsec;
#endif
}

bool os_mkdir(const char *path)
{
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

bool fs_rename(const char *from, const char *to)
{
    return rename(from, to) == 0;
}

void os_listdir(const char *dir, void (*fn)(void *ctx, const char *name, bool is_dir), void *ctx)
{
    DIR *d = opendir(dir);
    for (struct dirent *e = d ? readdir(d) : NULL; e; e = readdir(d)) {
        if (e->d_name[0] == '.') {
            continue; /* ., .. and hidden files */
        }
        char *path = str_fmt("%s/%s", dir, e->d_name);
        struct stat st;
        bool is_dir = stat(path, &st) == 0 && S_ISDIR(st.st_mode);
        free(path);
        fn(ctx, e->d_name, is_dir);
    }
    if (d) {
        closedir(d);
    }
}
