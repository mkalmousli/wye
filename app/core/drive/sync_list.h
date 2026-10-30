/*
 * Folder snapshots for the shared-folder sync: sorted path lists with size
 * and modification time, and the manifest (the state both sides agreed on
 * at the last sync) stored as text in the data directory.
 */
#ifndef WYE_SYNC_LIST_H
#define WYE_SYNC_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct sync_entry {
    char *path;                /* relative, '/' separated */
    bool dir;
    uint64_t size;
    int64_t here, there;       /* mtimes: this computer's file / the drive image's entry */
};

struct sync_list {
    struct sync_entry *v;
    size_t n, cap;
};

/* Snapshot of a folder; mtimes go to .here. */
void sync_list_scan(const char *root, struct sync_list *out);
void sync_list_add(struct sync_list *l, const char *path, bool dir, uint64_t size, int64_t here, int64_t there);
void sync_list_sort(struct sync_list *l);
const struct sync_entry *sync_list_find(const struct sync_list *l, const char *path);
void sync_list_free(struct sync_list *l);
bool sync_list_load(const char *file, struct sync_list *out);
bool sync_list_save(const char *file, const struct sync_list *l);

#endif
