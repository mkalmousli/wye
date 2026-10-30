/* FAT volumes: I/O, clusters, saving the FAT, allocating. */
#include "fat.h"
#include "wye_msg.h"
#include "util/util.h"
#include <stdlib.h>
#include <string.h>

bool fat_io(struct fat_vol *v, uint64_t offset, void *buf, size_t len, bool write)
{
    if (offset + len > (uint64_t)v->sectors * FAT_SECTOR || !wye_fseek64(v->f, v->base + offset)) {
        return false;
    }
    return write ? fwrite(buf, 1, len, v->f) == len : fread(buf, 1, len, v->f) == len;
}

uint64_t fat_cluster_offset(const struct fat_vol *v, uint32_t cluster)
{
    return ((uint64_t)v->data_lba * FAT_SECTOR) + (uint64_t)(cluster - 2) * v->cluster_bytes;
}

bool fat_eoc(const struct fat_vol *v, uint32_t entry)
{
    return v->fat32 ? entry >= 0x0ffffff8u : entry >= 0xfff8u;
}

bool fat_close(struct fat_vol *v)
{
    bool ok = true;
    size_t bytes = (size_t)v->fat_sectors * FAT_SECTOR, n = bytes / (v->fat32 ? 4 : 2);
    if (v->fat && v->dirty) {
        uint8_t *raw = malloc(bytes);
        for (size_t i = 0; raw && i < n; ++i) {
            v->fat32 ? wye_put_u32(raw + 4 * i, v->fat[i]) : wye_put_u16(raw + 2 * i, (uint16_t)v->fat[i]);
        }
        for (uint32_t copy = 0; raw && copy < 2 && ok; ++copy) {
            ok = fat_io(v, (uint64_t)(v->fat_lba + copy * v->fat_sectors) * FAT_SECTOR, raw, bytes, true);
        }
        if (ok && raw && v->fat32) { /* FSInfo: free clusters and where to look next, in both copies */
            uint8_t hint[8];
            wye_put_u32(hint, v->clusters + 2 - v->next_free);
            wye_put_u32(hint + 4, v->next_free);
            ok = fat_io(v, FAT_SECTOR + 488, hint, 8, true) && fat_io(v, 7 * FAT_SECTOR + 488, hint, 8, true);
        }
        ok = ok && raw && fflush(v->f) == 0;
        free(raw);
    }
    free(v->fat);
    v->fat = NULL;
    return ok;
}

uint32_t fat_alloc(struct fat_vol *v, uint32_t count)
{
    uint32_t first = v->next_free, eoc = v->fat32 ? 0x0fffffffu : 0xffffu;
    if (count == 0 || (uint64_t)first + count > (uint64_t)v->clusters + 2) {
        return 0;
    }
    for (uint32_t i = 0; i < count; ++i) {
        v->fat[first + i] = i + 1 < count ? first + i + 1 : eoc;
    }
    v->next_free += count;
    v->dirty = true;
    return first;
}
