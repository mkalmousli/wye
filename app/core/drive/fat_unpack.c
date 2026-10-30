/* Unpacking a FAT16 image into a folder (merging with what is there). */
#include "fat_tree.h"
#include "wye_msg.h"
#include "plat_sys.h"
#include <stdlib.h>
#include <string.h>

struct unpacker {
    struct fat_vol v;
    char *err;
    size_t errlen;
    uint8_t *cluster;
};

static bool unpack_dir(struct unpacker *u, uint32_t first, const char *path, int depth);

/* Handles one 32-byte entry; *end is set at the end-of-directory marker. */
static bool entry(struct unpacker *u, const uint8_t *e, struct fat_lfn *lfn, const char *path, int depth, bool *end)
{
    char name[300];
    *end = e[0] == 0;
    if (*end || e[0] == 0xe5 || (e[11] & FAT_ATTR_VOLUME) == FAT_ATTR_VOLUME) {
        if (e[11] == FAT_ATTR_LFN && e[0] != 0xe5 && !*end) {
            fat_lfn_add(lfn, e);
        } else {
            lfn->valid = false;
        }
        return true;
    }
    fat_entry_name(e, lfn, name, sizeof(name));
    lfn->valid = false;
    if (!name[0] || strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return true;
    }
    char *full = tree_join(path, name);
    uint32_t c = wye_get_u16(e + 26) | (u->v.fat32 ? (uint32_t)wye_get_u16(e + 20) << 16 : 0);
    bool ok = full != NULL;
    if (ok && (e[11] & FAT_ATTR_DIR)) {
        ok = depth < 32 && plat_mkdirs(full) && unpack_dir(u, c, full, depth + 1);
    } else if (ok) {
        ok = fat_copy_out(&u->v, full, c, wye_get_u32(e + 28), fat_entry_mtime(e), u->cluster, u->err, u->errlen);
    }
    free(full);
    return ok;
}

static bool unpack_dir(struct unpacker *u, uint32_t first, const char *path, int depth)
{
    struct fat_lfn lfn = {0};
    uint8_t *root = first ? NULL : malloc(FAT_ROOT_ENTRIES * FAT_DIRENT); /* only at the top: no deep stacks */
    bool ok = first || root, end = false;
    uint32_t c = first, guard = 0;
    while (ok && !end) {
        const uint8_t *buf = first ? u->cluster : root;
        size_t len = first ? u->v.cluster_bytes : FAT_ROOT_ENTRIES * FAT_DIRENT;
        uint64_t at = first ? fat_cluster_offset(&u->v, c) : (uint64_t)u->v.root_lba * FAT_SECTOR;
        if ((first && (!fat_valid_cluster(&u->v, c) || guard++ > u->v.clusters)) || !fat_io(&u->v, at, (void *)buf, len, false)) {
            break;
        }
        uint8_t *copy = malloc(len); /* nested calls reuse u->cluster */
        ok = copy != NULL;
        if (ok) {
            memcpy(copy, buf, len);
        }
        for (size_t i = 0; ok && !end && i < len; i += FAT_DIRENT) {
            ok = entry(u, copy + i, &lfn, path, depth, &end);
        }
        free(copy);
        if (!first) {
            break;
        }
        end = end || fat_eoc(&u->v, u->v.fat[c]);
        c = u->v.fat[c];
    }
    free(root);
    return ok;
}

bool fat_unpack(FILE *image, const char *folder, char *err, size_t errlen)
{
    struct unpacker u = {.err = err, .errlen = errlen};
    if (!fat_open(&u.v, image)) {
        snprintf(err, errlen, "The drive was reformatted on the other computer (it must stay FAT)");
        return false;
    }
    u.cluster = malloc(u.v.cluster_bytes);
    bool ok = u.cluster && plat_mkdirs(folder) && unpack_dir(&u, u.v.root_cluster, folder, 0); /* 0: FAT16 root */
    free(u.cluster);
    fat_close(&u.v);
    return ok;
}
