/*
 * FAT volume images for the shared drive: format, pack a folder into an
 * image and unpack an image into a folder. Images carry an MBR partition
 * table (Android insists on one) with a FAT16 (< 64 MB) or FAT32 volume at
 * 1 MiB. Older images without a table still open. Long file names are
 * written and read; file data is laid out in contiguous runs because every
 * pack starts from a freshly formatted volume.
 */
#ifndef WYE_FAT_H
#define WYE_FAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define FAT_SECTOR 512u
#define FAT_DIRENT 32u
#define FAT_ROOT_ENTRIES 512u
#define FAT_PART_LBA 2048u        /* the volume starts 1 MiB into the image */
#define FAT32_FROM_SECTORS (64u * 2048u)
#define FAT_ATTR_DIR 0x10u
#define FAT_ATTR_VOLUME 0x08u
#define FAT_ATTR_LFN 0x0fu

struct fat_vol {
    FILE *f;
    uint64_t base;                 /* byte offset of the volume inside the image */
    bool fat32;
    uint32_t sectors, fat_lba, fat_sectors, root_lba, data_lba;  /* volume-relative */
    uint32_t root_cluster;         /* FAT32: the root folder is a cluster chain */
    uint32_t clusters;             /* data clusters, numbered from 2 */
    uint32_t cluster_bytes;
    uint32_t *fat;                 /* first FAT, in memory (FAT16 entries widened) */
    uint32_t next_free;
    bool dirty;
};

/* fat_format.c / fat_format32.c: `sectors` is the whole image, partition table included */
bool fat_format(FILE *f, uint32_t sectors);
bool fat_format16(FILE *f, uint32_t sectors);
bool fat_format32(FILE *f, uint32_t sectors);
bool fat_write_at(FILE *f, uint64_t off, const void *buf, size_t len);
bool fat_write_zeros(FILE *f, uint64_t off, uint64_t len);

/* fat_vol.c */
bool fat_eoc(const struct fat_vol *v, uint32_t entry);
bool fat_open(struct fat_vol *v, FILE *f);
bool fat_close(struct fat_vol *v);                    /* writes both FATs if changed */
bool fat_io(struct fat_vol *v, uint64_t offset, void *buf, size_t len, bool write);
uint64_t fat_cluster_offset(const struct fat_vol *v, uint32_t cluster);
uint32_t fat_alloc(struct fat_vol *v, uint32_t count);  /* contiguous chain; 0 when full */

/* fat_name.c: long names and directory entries */
struct fat_lfn {
    uint16_t chars[260];
    uint8_t checksum, next;
    bool valid;
};
int fat_lfn_slots(const char *utf8);
/* Writes LFN slots + the short entry ((slots + 1) * 32 bytes) for one item. */
void fat_make_entries(uint8_t *out, const char *utf8, unsigned index, uint8_t attr, uint32_t cluster,
                      uint32_t size, time_t mtime);
void fat_dot_entries(uint8_t *out, uint32_t self, uint32_t parent);
void fat_lfn_add(struct fat_lfn *l, const uint8_t *entry);
/* Name of a short entry: its long name when one precedes it, else the 8.3 name. */
void fat_entry_name(const uint8_t *entry, const struct fat_lfn *l, char *out, size_t cap);
time_t fat_entry_mtime(const uint8_t *entry);           /* local time, 2 s resolution */

/* fat_copy.c */
bool fat_valid_cluster(const struct fat_vol *v, uint32_t c);
bool fat_copy_in(struct fat_vol *v, const char *path, uint32_t size, uint32_t *first, char *err, size_t errlen);
/* Also stamps the file with mtime (0 = leave it). */
bool fat_copy_out(struct fat_vol *v, const char *path, uint32_t first, uint32_t size, time_t mtime, uint8_t *buf,
                  char *err, size_t errlen);
bool fat_set_mtime(const char *path, time_t mtime);

/* fat_pack.c / fat_unpack.c */
bool fat_pack(const char *folder, FILE *image, uint32_t sectors, char *err, size_t errlen);
bool fat_unpack(FILE *image, const char *folder, char *err, size_t errlen);

#endif
