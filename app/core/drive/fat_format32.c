/* Formatting a FAT32 volume (64 MB and up) at 1 MiB into the image. */
#include "fat.h"
#include "wye_msg.h"
#include <string.h>

#define RESERVED 32u
#define VOL ((uint64_t)FAT_PART_LBA * FAT_SECTOR)

void fat_boot_common(uint8_t *b, uint32_t sectors, uint8_t spc, uint16_t reserved);

/* Cluster sizes as Windows picks them, which also keeps the FAT small in memory. */
static uint8_t cluster_sectors(uint32_t sectors)
{
    return sectors <= 532480u ? 1 : sectors <= 16777216u ? 8 : sectors <= 33554432u ? 16 : sectors <= 67108864u ? 32 : 64;
}

static void fsinfo(uint8_t *s)
{
    memset(s, 0, FAT_SECTOR);
    wye_put_u32(s, 0x41615252u);
    wye_put_u32(s + 484, 0x61417272u);
    wye_put_u32(s + 488, 0xffffffffu); /* free count unknown: the OS works it out */
    wye_put_u32(s + 492, 0xffffffffu);
    wye_put_u32(s + 508, 0xaa550000u);
}

bool fat_format32(FILE *f, uint32_t sectors)
{
    uint8_t spc = cluster_sectors(sectors), b[FAT_SECTOR], info[FAT_SECTOR], label[FAT_DIRENT] = {0};
    uint32_t fat_sectors = ((sectors - RESERVED) / spc + 2) * 4 / FAT_SECTOR + 1;
    uint32_t clusters = (sectors - RESERVED - 2 * fat_sectors) / spc;
    if (clusters < 65525u) {
        return false;
    }
    fat_boot_common(b, sectors, spc, RESERVED);
    memcpy(b, "\xeb\x58\x90", 3);
    wye_put_u32(b + 36, fat_sectors);
    wye_put_u32(b + 44, 2);                           /* root folder cluster */
    wye_put_u16(b + 48, 1);                           /* FSInfo sector */
    wye_put_u16(b + 50, 6);                           /* backup boot sector */
    b[64] = 0x80;
    b[66] = 0x29;
    wye_put_u32(b + 67, (uint32_t)time(NULL));
    memcpy(b + 71, "WYE        FAT32   ", 19);
    fsinfo(info);
    uint8_t fat[12];
    wye_put_u32(fat, 0x0ffffff8u);
    wye_put_u32(fat + 4, 0x0fffffffu);
    wye_put_u32(fat + 8, 0x0fffffffu);                /* the root folder: one cluster */
    memcpy(label, "WYE        ", 11);
    label[11] = FAT_ATTR_VOLUME;
    uint64_t data = VOL + (uint64_t)(RESERVED + 2 * fat_sectors) * FAT_SECTOR;
    bool ok = fat_write_zeros(f, VOL, (uint64_t)(RESERVED + 2 * fat_sectors) * FAT_SECTOR + spc * FAT_SECTOR);
    for (uint32_t copy = 0; ok && copy < 2; ++copy) { /* boot + FSInfo, and their backups at 6/7 */
        ok = fat_write_at(f, VOL + copy * 6u * FAT_SECTOR, b, sizeof(b)) &&
             fat_write_at(f, VOL + (copy * 6u + 1) * FAT_SECTOR, info, sizeof(info)) &&
             fat_write_at(f, VOL + (uint64_t)(RESERVED + copy * fat_sectors) * FAT_SECTOR, fat, sizeof(fat));
    }
    return ok && fat_write_at(f, data, label, sizeof(label));
}
