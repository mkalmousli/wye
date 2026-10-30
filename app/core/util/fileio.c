#define _POSIX_C_SOURCE 200809L /* fseeko/ftello */
#define _FILE_OFFSET_BITS 64
#include "util.h"

#include "plat_sys.h"

#include <stdio.h>
#include <stdlib.h>

uint8_t *wye_read_file(const char *path, size_t *len, size_t max)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    uint8_t *data = NULL;
    long size = -1;
    if (fseek(f, 0, SEEK_END) == 0) {
        size = ftell(f);
    }
    if (size >= 0 && (size_t)size <= max && fseek(f, 0, SEEK_SET) == 0) {
        data = malloc((size_t)size + 1);
    }
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

bool wye_write_file_atomic(const char *path, const void *data, size_t len)
{
    char tmp[PLAT_PATH_MAX];
    if (!wye_strcpy(tmp, sizeof(tmp), path) || !wye_strcat(tmp, sizeof(tmp), ".tmp")) {
        return false;
    }
    FILE *f = fopen(tmp, "wb");
    if (!f) {
        return false;
    }
    bool ok = fwrite(data, 1, len, f) == len;
    ok = fflush(f) == 0 && ok;
    ok = fclose(f) == 0 && ok;
    if (ok) {
        ok = plat_rename_replace(tmp, path);
    }
    if (!ok) {
        remove(tmp);
    }
    return ok;
}

bool wye_fseek64(FILE *f, uint64_t offset)
{
#if defined(_WIN32)
    return _fseeki64(f, (long long)offset, SEEK_SET) == 0;
#else
    return fseeko(f, (off_t)offset, SEEK_SET) == 0;
#endif
}

int64_t wye_ftell64(FILE *f)
{
#if defined(_WIN32)
    return _ftelli64(f);
#else
    return (int64_t)ftello(f);
#endif
}
