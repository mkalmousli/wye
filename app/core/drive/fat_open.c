/* Opening a FAT16/FAT32 image, with or without a partition table: geometry and the FAT. */
#include "fat.h"
#include "wye_msg.h"
#include "util/util.h"
#include <stdlib.h>
#include <string.h>

/* The boot sector, at 0 (no partition table) or where the first FAT partition starts. */
static bool find_volume(struct fat_vol *v, uint8_t *b)
{
    if (!wye_fseek64(v->f, 0) || fread(b, 1, FAT_SECTOR, v->f) != FAT_SECTOR || b[510] != 0x55 || b[511] != 0xaa) {
        return false;
    }
    bool bpb = (b[0] == 0xeb || b[0] == 0xe9) && wye_get_u16(b + 11) == FAT_SECTOR && b[13] && b[16];
    uint8_t type = b[446 + 4];
    if (!bpb && (type == 0x01 || type == 0x04 || type == 0x06 || type == 0x0b || type == 0x0c || type == 0x0e)) {
        v->base = (uint64_t)wye_get_u32(b + 446 + 8) * FAT_SECTOR;
        return wye_fseek64(v->f, v->base) && fread(b, 1, FAT_SECTOR, v->f) == FAT_SECTOR &&
               wye_get_u16(b + 11) == FAT_SECTOR && b[13] && b[16];
    }
    return bpb;
}

static bool load_fat(struct fat_vol *v)
{
    size_t bytes = (size_t)v->fat_sectors * FAT_SECTOR, n = bytes / (v->fat32 ? 4 : 2);
    uint8_t *raw = malloc(bytes);
    v->fat = malloc(n * sizeof(uint32_t));
    bool ok = raw && v->fat && fat_io(v, (uint64_t)v->fat_lba * FAT_SECTOR, raw, bytes, false);
    for (size_t i = 0; ok && i < n; ++i) {
        v->fat[i] = v->fat32 ? wye_get_u32(raw + 4 * i) & 0x0fffffffu : wye_get_u16(raw + 2 * i);
    }
    free(raw);
    if (!ok) {
        free(v->fat);
        v->fat = NULL;
    }
    return ok;
}

bool fat_open(struct fat_vol *v, FILE *f)
{
    uint8_t b[FAT_SECTOR];
    memset(v, 0, sizeof(*v));
    v->f = f;
    if (!find_volume(v, b)) {
        return false;
    }
    uint32_t spc = b[13], reserved = wye_get_u16(b + 14), root_entries = wye_get_u16(b + 17);
    v->sectors = wye_get_u16(b + 19) ? wye_get_u16(b + 19) : wye_get_u32(b + 32);
    v->fat32 = wye_get_u16(b + 22) == 0;
    v->fat_sectors = v->fat32 ? wye_get_u32(b + 36) : wye_get_u16(b + 22);
    v->root_cluster = v->fat32 ? wye_get_u32(b + 44) : 0;
    v->fat_lba = reserved;
    v->root_lba = reserved + b[16] * v->fat_sectors;
    v->data_lba = v->root_lba + root_entries * FAT_DIRENT / FAT_SECTOR;
    v->cluster_bytes = spc * FAT_SECTOR;
    v->clusters = v->sectors > v->data_lba ? (v->sectors - v->data_lba) / spc : 0;
    bool fat16_ok = !v->fat32 && v->clusters >= 4085 && v->clusters <= 65524 && root_entries == FAT_ROOT_ENTRIES;
    bool fat32_ok = v->fat32 && v->clusters >= 65525 && root_entries == 0 && v->root_cluster >= 2;
    v->next_free = 2;
    return (fat16_ok || fat32_ok) && load_fat(v); /* else reformatted (exFAT/NTFS?) */
}
