/* Portable file helpers built on the OS primitives in os_*.c. */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool fs_exists(const char *path)
{
    return fs_mtime(path) >= 0;
}

bool fs_mkdirs(const char *path)
{
    char *buf = str_fmt("%s", path);
    for (char *p = buf + 1; *p; ++p) {
        if ((*p == '/' || *p == '\\') && p[-1] != ':') {
            char c = *p;
            *p = '\0';
            os_mkdir(buf);
            *p = c;
        }
    }
    bool ok = os_mkdir(buf);
    free(buf);
    return ok;
}

bool fs_mkdir_parent(const char *file)
{
    char *dir = str_fmt("%s", file);
    char *slash = strrchr(dir, '/');
    bool ok = !slash || (*slash = '\0', fs_mkdirs(dir));
    free(dir);
    return ok;
}

char *fs_read(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = size >= 0 ? malloc((size_t)size + 1) : NULL;
    if (data && fread(data, 1, (size_t)size, f) != (size_t)size) {
        free(data);
        data = NULL;
    }
    fclose(f);
    if (data) {
        data[size] = '\0';
        if (len) {
            *len = (size_t)size;
        }
    }
    return data;
}

/* Leaves the file (and its mtime) alone when the content is identical. */
bool fs_write_if_changed(const char *path, const void *data, size_t len)
{
    size_t old_len = 0;
    char *old = fs_read(path, &old_len);
    bool same = old && old_len == len && memcmp(old, data, len) == 0;
    free(old);
    if (same) {
        return true;
    }
    FILE *f = fs_mkdir_parent(path) ? fopen(path, "wb") : NULL;
    if (!f) {
        fprintf(stderr, "build: cannot write %s\n", path);
        return false;
    }
    bool ok = fwrite(data, 1, len, f) == len;
    return fclose(f) == 0 && ok;
}

bool fs_copy(const char *from, const char *to)
{
    size_t len = 0;
    char *data = fs_read(from, &len);
    bool ok = data && fs_write_if_changed(to, data, len);
    free(data);
    return ok;
}
