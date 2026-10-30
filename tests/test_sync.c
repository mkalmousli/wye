/* Shared-folder sync decisions: copy their changes in, notice ours, keep both on a clash. */
#include "drive/drive_sync.h"
#include "drive/fat_tree.h"
#include "plat_sys.h"
#include "test.h"
#include "util/util.h"

#include <stdlib.h>

#define ROOT "out/test-sync"
#define T0 1700000000

static void put(const char *path, const char *text, int64_t mtime)
{
    wye_write_file_atomic(path, text, strlen(text));
    fat_set_mtime(path, (time_t)mtime);
}

static bool has(const char *path, const char *text)
{
    char *got = (char *)wye_read_file(path, NULL, 256);
    bool ok = got && strcmp(got, text) == 0;
    free(got);
    return ok;
}

TEST(sync_copies_theirs_notices_ours_keeps_both)
{
    struct sync_list manifest = {0}, staged;
    struct sync_dirs dirs = {ROOT "/here", ROOT "/there"};
    tree_remove(ROOT);
    CHECK(plat_mkdirs(ROOT "/here") && plat_mkdirs(ROOT "/there"));
    /* Agreed state: a, c and d exist on both sides. */
    const char *names[] = {"a.txt", "c.txt", "d.txt"};
    for (int i = 0; i < 3; ++i) {
        char *h = tree_join(dirs.folder, names[i]), *t = tree_join(dirs.staging, names[i]);
        put(h, "same", T0);
        put(t, "same", T0);
        sync_list_add(&manifest, names[i], false, 4, T0, T0);
        free(h);
        free(t);
    }
    sync_list_sort(&manifest);
    sync_list_scan(dirs.staging, &staged);
    CHECK(!drive_sync_apply(&dirs, &staged, &manifest)); /* nothing changed */
    sync_list_free(&staged);
    put(ROOT "/there/a.txt", "theirs", T0 + 10);   /* edited there */
    remove(ROOT "/there/c.txt");                    /* deleted there */
    put(ROOT "/here/d.txt", "mine!", T0 + 20);     /* edited on both sides */
    put(ROOT "/there/d.txt", "their", T0 + 22);
    sync_list_scan(dirs.staging, &staged);
    CHECK(drive_sync_apply(&dirs, &staged, &manifest)); /* d.txt must go back to the image */
    CHECK(has(ROOT "/here/a.txt", "theirs"));
    CHECK(!plat_path_exists(ROOT "/here/c.txt"));
    CHECK(has(ROOT "/here/d.txt", "mine!"));
    CHECK(has(ROOT "/here/d (other computer).txt", "their"));
    sync_list_free(&staged);
    put(ROOT "/here/new.txt", "x", T0 + 30);       /* new here, image unchanged */
    CHECK(drive_sync_apply(&dirs, NULL, &manifest));
    sync_list_free(&manifest);
    tree_remove(ROOT);
}
