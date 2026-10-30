/*
 * Two-way sync between the shared folder here and the drive image: every
 * path is compared against the manifest (what both sides agreed on last
 * time). Changes made on the other computer are copied in; changes made here
 * make the caller rebuild the image. When both sides changed one file, the
 * other computer's version is kept next to it as "name (other computer).ext".
 */
#include "drive_sync.h"
#include "fat_tree.h"
#include <stdlib.h>
#include <string.h>

static bool changed(const struct sync_entry *now, const struct sync_entry *m, bool there)
{
    if (!now || !m) {
        return now != m; /* appeared or vanished */
    }
    if (now->dir || m->dir) {
        return now->dir != m->dir;
    }
    return now->size != m->size || now->here != (there ? m->there : m->here);
}

char *drive_conflict_name(const char *path);
void drive_take(const struct sync_dirs *d, const struct sync_entry *s, const char *as);
void drive_drop(const struct sync_dirs *d, const struct sync_entry *h, struct sync_list *dirs);

/* One path: returns true if this computer has something the image lacks. */
static bool one(const struct sync_dirs *d, const struct sync_entry *s, const struct sync_entry *m,
                const struct sync_entry *h, struct sync_list *dirs)
{
    bool there = changed(s, m, true), here = changed(h, m, false);
    if (there && !here) {
        if (s) {
            drive_take(d, s, NULL);
        } else if (h) {
            drive_drop(d, h, dirs);
        }
        return false;
    }
    if (there && here && s && h && !s->dir && !h->dir) {
        char *other = drive_conflict_name(s->path);
        drive_take(d, s, other); /* both kept; ours goes back to the image */
        free(other);
        return true;
    }
    if (there && here && s && !h) {
        drive_take(d, s, NULL); /* deleted here, edited there: keep the edit */
        return false;
    }
    return here;
}

bool drive_sync_apply(const struct sync_dirs *d, const struct sync_list *staged, const struct sync_list *manifest)
{
    struct sync_list here, dirs = {0};
    sync_list_scan(d->folder, &here);
    bool need_pack = false;
    const struct sync_list *lists[3] = {staged, manifest, &here};
    size_t at[3] = {0, 0, 0};
    for (;;) { /* merge-walk the three sorted lists */
        const char *next = NULL;
        for (int i = 0; i < 3; ++i) {
            if (lists[i] && at[i] < lists[i]->n && (!next || strcmp(lists[i]->v[at[i]].path, next) < 0)) {
                next = lists[i]->v[at[i]].path;
            }
        }
        if (!next) {
            break;
        }
        const struct sync_entry *e[3] = {NULL, NULL, NULL};
        for (int i = 0; i < 3; ++i) {
            if (lists[i] && at[i] < lists[i]->n && strcmp(lists[i]->v[at[i]].path, next) == 0) {
                e[i] = &lists[i]->v[at[i]++];
            }
        }
        /* Without a fresh image snapshot the image is as the manifest says. */
        need_pack |= staged ? one(d, e[0], e[1], e[2], &dirs) : changed(e[2], e[1], false);
    }
    for (size_t i = dirs.n; i-- > 0;) { /* children were listed after their parents */
        need_pack |= !tree_rmdir(dirs.v[i].path); /* still has files from here: the image gets them back */
    }
    sync_list_free(&dirs);
    sync_list_free(&here);
    return need_pack;
}
