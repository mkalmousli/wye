/* Formatting: the partition table and a FAT16 volume (small drives). */
#include "fat.h"
#include "wye_msg.h"
#include "plat_sys.h"
#include "util/util.h"
#include <stdlib.h>
#include <string.h>

#define RESERVED 4u
#define ROOT_SECTORS (FAT_ROOT_ENTRIES * FAT_DIRENT / FAT_SECTOR)
#define VOL ((uint64_t)FAT_PART_LBA * FAT_SECTOR)

bool fat_write_at(FILE *f, uint64_t off, const void *buf, size_t len)
{
    return wye_fseek64(f, off) && fwrite(buf, 1, len, f) == len;
}

bool fat_write_zeros(FILE *f, uint64_t off, uint64_t len)
{
    static const uint8_t zero[65536];
    bool ok = wye_fseek64(f, off);
    for (uint64_t done = 0; ok && done < len; done += sizeof(zero)) {
        size_t n = len - done < sizeof(zero) ? (size_t)(len - done) : sizeof(zero);
        ok = fwrite(zero, 1, n, f) == n;
    }
    return ok;
}

/* One primary partition from 1 MiB to the end: 0x0e FAT16 LBA or 0x0c FAT32 LBA. */
static bool partition_table(FILE *f, uint32_t sectors, bool fat32)
{
    uint8_t mbr[FAT_SECTOR] = {0};
    uint8_t *p = mbr + 446;
    p[1] = p[5] = 0xfe; /* CHS "use LBA" */
    p[2] = p[3] = p[6] = p[7] = 0xff;
    p[4] = fat32 ? 0x0c : 0x0e;
    wye_put_u32(p + 8, FAT_PART_LBA);
    wye_put_u32(p + 12, sectors - FAT_PART_LBA);
    mbr[510] = 0x55;
    mbr[511] = 0xaa;
    return fat_write_at(f, 0, mbr, sizeof(mbr));
}

void fat_boot_common(uint8_t *b, uint32_t sectors, uint8_t spc, uint16_t reserved)
{
    memset(b, 0, FAT_SECTOR);
    memcpy(b + 3, "WYE     ", 8);
    wye_put_u16(b + 11, FAT_SECTOR);
    b[13] = spc;
    wye_put_u16(b + 14, reserved);
    b[16] = 2;                                        /* FAT copies */
    b[21] = 0xf8;                                     /* fixed media */
    wye_put_u16(b + 24, 63);
    wye_put_u16(b + 26, 255);
    wye_put_u32(b + 28, FAT_PART_LBA);                /* hidden sectors before the volume */
    wye_put_u32(b + 32, sectors >= 65536u ? sectors : 0);
    wye_put_u16(b + 19, sectors < 65536u ? (uint16_t)sectors : 0);
    b[510] = 0x55;
    b[511] = 0xaa;
}

bool fat_format16(FILE *f, uint32_t sectors)
{
    uint8_t spc = 1, b[FAT_SECTOR], fat[4] = {0xf8, 0xff, 0xff, 0xff}, label[FAT_DIRENT] = {0};
    while (spc < 64 && sectors / spc > 65000u) {
        spc = (uint8_t)(spc * 2);
    }
    uint32_t fat_sectors = ((sectors - RESERVED - ROOT_SECTORS) / spc + 2) * 2 / FAT_SECTOR + 1;
    uint32_t clusters = (sectors - RESERVED - 2 * fat_sectors - ROOT_SECTORS) / spc;
    if (clusters < 4085 || clusters > 65524) {
        return false;
    }
    fat_boot_common(b, sectors, spc, RESERVED);
    memcpy(b, "\xeb\x3c\x90", 3);
    wye_put_u16(b + 17, FAT_ROOT_ENTRIES);
    wye_put_u16(b + 22, (uint16_t)fat_sectors);
    b[36] = 0x80;
    b[38] = 0x29;                                     /* extended boot signature */
    wye_put_u32(b + 39, (uint32_t)time(NULL));        /* volume serial */
    memcpy(b + 43, "WYE        FAT16   ", 19);
    memcpy(label, "WYE        ", 11);
    label[11] = FAT_ATTR_VOLUME;
    uint64_t root = VOL + (uint64_t)(RESERVED + 2 * fat_sectors) * FAT_SECTOR;
    return fat_write_zeros(f, VOL, (uint64_t)(RESERVED + 2 * fat_sectors + ROOT_SECTORS) * FAT_SECTOR) &&
           fat_write_at(f, VOL, b, sizeof(b)) && fat_write_at(f, VOL + RESERVED * FAT_SECTOR, fat, 4) &&
           fat_write_at(f, VOL + (uint64_t)(RESERVED + fat_sectors) * FAT_SECTOR, fat, 4) &&
           fat_write_at(f, root, label, sizeof(label));
}

bool fat_format(FILE *f, uint32_t sectors)
{
    bool fat32 = sectors >= FAT32_FROM_SECTORS;
    uint8_t last = 0;
    plat_file_sparse(f); /* a 32 GB image must not cost 32 GB until it is used */
    return partition_table(f, sectors, fat32) &&
           (fat32 ? fat_format32(f, sectors - FAT_PART_LBA) : fat_format16(f, sectors - FAT_PART_LBA)) &&
           fat_write_at(f, (uint64_t)sectors * FAT_SECTOR - 1, &last, 1) && fflush(f) == 0;
}
