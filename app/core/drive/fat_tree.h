/* Walking real folders for the FAT packer (dirent/stat, also on MinGW). */
#ifndef WYE_FAT_TREE_H
#define WYE_FAT_TREE_H

#include "fat.h"

struct tree_names {
    char **items;
    size_t count;
};

struct tree_item {
    bool is_dir;
    uint64_t size;
    time_t mtime;
};

/* Names of the files and folders in path (no "." / ".."); false if unreadable. */
bool tree_list(const char *path, struct tree_names *out);
void tree_free(struct tree_names *n);
/* False for things that cannot go on the drive (sockets, devices, broken links). */
bool tree_stat(const char *path, struct tree_item *out);
/* Directory entries a folder needs, including "." and ".." when sub is true. */
int tree_slots(const char *path, bool sub);
char *tree_join(const char *dir, const char *name);   /* malloc'd */
bool tree_remove(const char *path);                    /* recursive delete */
bool tree_rmdir(const char *path);                     /* only if empty */

#endif
