/* Packing a folder into a freshly formatted FAT16 image. */
#include "fat_tree.h"
#include <stdlib.h>
#include <string.h>

struct packer {
    struct fat_vol v;
    char *err;
    size_t errlen;
};

static bool fail(struct packer *p, const char *msg, const char *path)
{
    snprintf(p->err, p->errlen, "%s: %s", msg, path);
    return false;
}

static bool pack_dir(struct packer *p, const char *path, uint32_t self, uint32_t parent, int depth, bool root);

/* One item: its data (or sub-folder) goes into the image, its entries into `out`. */
static bool add_item(struct packer *p, const char *full, const char *name, unsigned index, uint32_t parent,
                     int depth, uint8_t *out)
{
    struct tree_item it;
    if (!tree_stat(full, &it)) {
        return true;
    }
    uint32_t first = 0;
    if (it.is_dir) {
        uint32_t bytes = (uint32_t)tree_slots(full, true) * FAT_DIRENT;
        first = fat_alloc(&p->v, (bytes + p->v.cluster_bytes - 1) / p->v.cluster_bytes);
        if (!first || depth > 32) {
            return fail(p, first ? "Folders nested too deeply" : "Not enough space on the drive for", full);
        }
        if (!pack_dir(p, full, first, parent, depth + 1, false)) {
            return false;
        }
    } else if (it.size > 0xffffffffull) {
        return fail(p, "Files over 4 GB do not fit on the drive", full);
    } else if (!fat_copy_in(&p->v, full, (uint32_t)it.size, &first, p->err, p->errlen)) {
        return false;
    }
    fat_make_entries(out, name, index, it.is_dir ? FAT_ATTR_DIR : 0x20, first, it.is_dir ? 0 : (uint32_t)it.size,
                     it.mtime);
    return true;
}

/* Writes one folder's entries; self 0 = the FAT16 root region, root = no "." / ".." but the label. */
static bool pack_dir(struct packer *p, const char *path, uint32_t self, uint32_t parent, int depth, bool root)
{
    struct tree_names names;
    uint32_t cb = p->v.cluster_bytes;
    size_t area = self ? (size_t)((tree_slots(path, !root) * FAT_DIRENT + FAT_DIRENT + cb - 1) / cb * cb)
                       : FAT_ROOT_ENTRIES * FAT_DIRENT;
    uint8_t *buf = calloc(1, area);
    bool ok = buf && (tree_list(path, &names) || (free(buf), buf = NULL, fail(p, "Cannot read", path)));
    size_t pos = root ? FAT_DIRENT : 2 * FAT_DIRENT;
    if (ok && root) {
        memcpy(buf, "WYE        ", 11); /* the volume label the other computer shows */
        buf[11] = FAT_ATTR_VOLUME;
    } else if (ok) {
        fat_dot_entries(buf, self, parent);
    }
    for (size_t i = 0; ok && i < names.count; ++i) {
        char *full = tree_join(path, names.items[i]);
        size_t need = (size_t)(fat_lfn_slots(names.items[i]) + 1) * FAT_DIRENT;
        if (pos + need > area) {
            ok = fail(p, "Too many items at the top of the drive folder (move some into a sub-folder)", path);
        } else if ((ok = add_item(p, full, names.items[i], (unsigned)i, root ? 0 : self, depth, buf + pos)) &&
                   buf[pos]) {
            pos += need;
        }
        free(full);
    }
    uint64_t where = self ? fat_cluster_offset(&p->v, self) : (uint64_t)p->v.root_lba * FAT_SECTOR;
    ok = ok && fat_io(&p->v, where, buf, area, true);
    if (buf) {
        tree_free(&names);
    }
    free(buf);
    return ok;
}

bool fat_pack(const char *folder, FILE *image, uint32_t sectors, char *err, size_t errlen)
{
    struct packer p = {.err = err, .errlen = errlen};
    if (!fat_format(image, sectors) || !fat_open(&p.v, image)) {
        snprintf(err, errlen, "Could not format the drive image");
        return false;
    }
    uint32_t root = 0;
    if (p.v.fat32) { /* the root is a chain like any folder, starting at its fixed cluster */
        uint32_t bytes = (uint32_t)(tree_slots(folder, false) + 1) * FAT_DIRENT;
        root = fat_alloc(&p.v, (bytes + p.v.cluster_bytes - 1) / p.v.cluster_bytes);
    }
    bool ok = (!p.v.fat32 || root == p.v.root_cluster) && pack_dir(&p, folder, root, 0, 0, true);
    return fat_close(&p.v) && ok;
}
