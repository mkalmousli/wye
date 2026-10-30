/* Copying file data into and out of a FAT16 image. */
#include "fat.h"
#if defined(_WIN32)
#include <sys/utime.h>
#else
#include <utime.h>
#endif

static bool fail(char *err, size_t errlen, const char *msg, const char *path)
{
    snprintf(err, errlen, "%s: %s", msg, path);
    return false;
}

bool fat_valid_cluster(const struct fat_vol *v, uint32_t c)
{
    return c >= 2 && c < v->clusters + 2;
}

/* Into a contiguous run of fresh clusters (packing always starts from a new volume). */
bool fat_copy_in(struct fat_vol *v, const char *path, uint32_t size, uint32_t *first, char *err, size_t errlen)
{
    static uint8_t buf[65536];
    uint32_t n = (size + v->cluster_bytes - 1) / v->cluster_bytes;
    *first = size ? fat_alloc(v, n) : 0;
    if (size && !*first) {
        return fail(err, errlen, "Not enough space on the drive for", path);
    }
    FILE *in = size ? fopen(path, "rb") : NULL;
    if (size && !in) {
        return fail(err, errlen, "Cannot read", path);
    }
    uint64_t at = size ? fat_cluster_offset(v, *first) : 0;
    bool ok = true;
    for (uint32_t done = 0; ok && done < size;) {
        size_t chunk = size - done < sizeof(buf) ? size - done : sizeof(buf);
        ok = fread(buf, 1, chunk, in) == chunk && fat_io(v, at + done, buf, chunk, true);
        done += (uint32_t)chunk;
    }
    if (in) {
        fclose(in);
    }
    return ok || fail(err, errlen, "Could not copy", path);
}

/* Out, following the cluster chain. */
bool fat_copy_out(struct fat_vol *v, const char *path, uint32_t c, uint32_t size, time_t mtime, uint8_t *buf,
                  char *err, size_t errlen)
{
    FILE *out = fopen(path, "wb");
    bool ok = out != NULL;
    for (uint32_t left = size, guard = 0; ok && left > 0; c = v->fat[c], ++guard) {
        uint32_t n = left < v->cluster_bytes ? left : v->cluster_bytes;
        ok = fat_valid_cluster(v, c) && guard <= v->clusters &&
             fat_io(v, fat_cluster_offset(v, c), buf, n, false) && fwrite(buf, 1, n, out) == n;
        left -= n;
    }
    if (out) {
        ok = fclose(out) == 0 && ok;
    }
    ok = ok && (!mtime || fat_set_mtime(path, mtime));
    if (!ok) {
        fail(err, errlen, "Could not unpack", path);
    }
    return ok;
}

bool fat_set_mtime(const char *path, time_t mtime)
{
    struct utimbuf t = {mtime, mtime};
    return utime(path, &t) == 0;
}
