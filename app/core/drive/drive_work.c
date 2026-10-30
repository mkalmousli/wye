/* The work itself: packing the folder into the image, and syncing the two. */
#include "drive_work.h"
#include "drive_sync.h"
#include "fat_tree.h"
#include <string.h>

/* Records the folder as both sides have it now. `there` gives the image's times (NULL: just packed). */
static void save_manifest(struct drive_work *w, const struct sync_list *there)
{
    struct sync_list now;
    sync_list_scan(w->folder, &now);
    for (size_t i = 0; i < now.n; ++i) {
        const struct sync_entry *t = there ? sync_list_find(there, now.v[i].path) : NULL;
        now.v[i].there = t ? t->here : now.v[i].here & ~(int64_t)1; /* FAT keeps even seconds */
    }
    sync_list_save(w->manifest, &now);
    sync_list_free(&now);
}

void drive_work_pack(struct drive_work *w)
{
    char tmp[PLAT_PATH_MAX + 8];
    snprintf(tmp, sizeof(tmp), "%s.new", w->image);
    FILE *f = fopen(tmp, "w+b");
    w->r.ok = f && fat_pack(w->folder, f, w->sectors, w->r.err, sizeof(w->r.err));
    w->r.ok = f && fclose(f) == 0 && w->r.ok;
    if (!f) {
        snprintf(w->r.err, sizeof(w->r.err), "Cannot create the drive image");
    }
    w->r.ok = w->r.ok && plat_rename_replace(tmp, w->image); /* the old image stays intact on failure */
    if (w->r.ok) {
        save_manifest(w, NULL);
    } else {
        remove(tmp);
    }
}

void drive_work_sync(struct drive_work *w)
{
    struct sync_list manifest, staged = {0};
    struct sync_dirs dirs = {w->folder, w->staging};
    sync_list_load(w->manifest, &manifest);
    w->r.ok = true;
    if (w->read_image) {
        FILE *f = fopen(w->image, "rb");
        tree_remove(w->staging);
        w->r.ok = f && fat_unpack(f, w->staging, w->r.err, sizeof(w->r.err));
        if (f) {
            fclose(f);
        }
        sync_list_scan(w->staging, &staged);
    }
    if (w->r.ok) {
        w->r.need_pack = drive_sync_apply(&dirs, w->read_image ? &staged : NULL, &manifest);
        if (w->read_image && !w->r.need_pack) {
            save_manifest(w, &staged);
        }
    }
    sync_list_free(&staged);
    sync_list_free(&manifest);
}
