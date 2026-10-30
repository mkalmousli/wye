/* Shared-folder sync: copying in, deleting, naming conflicts. */
#include "drive_sync.h"
#include "fat_tree.h"
#include "plat_sys.h"
#include <stdlib.h>
#include <string.h>

char *drive_conflict_name(const char *path)
{
    const char *slash = strrchr(path, '/'), *dot = strrchr(path, '.');
    size_t stem = dot && (!slash || dot > slash + 1) ? (size_t)(dot - path) : strlen(path);
    char *out = malloc(strlen(path) + 20);
    if (out) {
        memcpy(out, path, stem);
        strcpy(out + stem, " (other computer)");
        strcat(out, path + stem);
    }
    return out;
}

/* Copies a file of the unpacked image into the folder, keeping its time. */
void drive_take(const struct sync_dirs *d, const struct sync_entry *s, const char *as)
{
    char *from = tree_join(d->staging, s->path), *to = tree_join(d->folder, as ? as : s->path);
    char *parent = to ? tree_join(to, "") : NULL;
    if (parent) {
        *strrchr(parent, '/') = '\0';  /* drop the separator tree_join added... */
        *strrchr(parent, '/') = '\0';  /* ...and the file name: its folder remains */
    }
    if (from && to && parent && plat_mkdirs(parent)) {
        if (s->dir) {
            plat_mkdirs(to);
        } else if (plat_rename_replace(from, to)) { /* staging is rebuilt every time: moving is fine */
            fat_set_mtime(to, (time_t)s->here);
        }
    }
    free(from);
    free(to);
    free(parent);
}

/* Files go at once; folders after the walk, deepest first (see drive_sync_apply). */
void drive_drop(const struct sync_dirs *d, const struct sync_entry *h, struct sync_list *dirs)
{
    char *path = tree_join(d->folder, h->path);
    if (path && h->dir) {
        sync_list_add(dirs, path, true, 0, 0, 0);
    } else if (path) {
        remove(path);
    }
    free(path);
}
