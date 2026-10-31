/* Shared folder end to end: host drive <-> bridge relay <-> target USB drive, both ways. */
#include "drive/fat.h"
#include <stdio.h>
#include <string.h>
#include "drive/fat_tree.h"
#include "drive_harness.h"
#include "plat_thread.h"
#include "test.h"
#include "util/util.h"

#include <stdlib.h>
#include <string.h>

#define ROOT DRIVE_TEST_ROOT

/* What the other computer does: rewrite the drive's files, as if through its USB drive. */
static void edit_there(const char *other)
{
    FILE *img = fopen(g_drive.image, "r+b");
    char err[160];
    CHECK(img && fat_pack(other, img, 16u * 2048u, err, sizeof(err)));
    if (img) {
        fclose(img);
    }
    g_drive.writes++; /* the block writes it would have made */
}

static bool exists_in_image(const char *name)
{
    FILE *img = fopen(g_drive.image, "rb");
    char err[160];
    bool ok = img && fat_unpack(img, ROOT "/peek", err, sizeof(err));
    char *path = tree_join(ROOT "/peek", name);
    ok = ok && plat_path_exists(path);
    free(path);
    if (img) {
        fclose(img);
    }
    tree_remove(ROOT "/peek");
    return ok;
}

TEST(drive_folder_syncs_both_ways)
{
    uint8_t block[WYE_DISK_BLOCK];
    tree_remove(ROOT);
    CHECK(plat_mkdirs(ROOT "/data") && plat_mkdirs(ROOT "/folder") && plat_mkdirs(ROOT "/other"));
    CHECK(test_use_data_dir(ROOT "/data")); /* never the user's real drive image */
    wye_write_file_atomic(ROOT "/folder/hello.txt", "hello", 5);
    sim_init(&g_sim);
    wye_drive_init(&g_drive, true, ROOT "/folder", 16);
    char image[1024]; /* Windows paths use backslashes: compare with '/' */
    size_t n = 0;
    for (; n + 1 < sizeof(image) && g_drive.image[n]; ++n) {
        image[n] = g_drive.image[n];
    }
    image[n] = 0;
    for (char *c = image; *c; ++c) {
        if (*c == '\\') {
            *c = '/';
        }
    }
    if (!strstr(image, ROOT "/data")) {
        CHECK(!"drive image would land outside the test folder");
        return;
    }
    wait_state(WYE_DRIVE_SYNCED);
    CHECK_EQ(g_drive.state, WYE_DRIVE_SYNCED);
    CHECK(wye_disk_ready(&g_sim.target.disk));
    int32_t r = 0;
    for (int i = 0; i < 200 && (r = wye_disk_read(&g_sim.target, 0, block)) == 0; ++i) {
        pump(2); /* what the target firmware's USB callback does while it waits */
    }
    CHECK(r == (int32_t)WYE_DISK_BLOCK && block[510] == 0x55 && block[511] == 0xaa);
    /* A change here: the drive goes out briefly and comes back with it. */
    wye_write_file_atomic(ROOT "/folder/mine.txt", "mine", 4);
    g_saw_removing = false;
    for (int i = 0; i < 400 && !g_saw_removing; ++i) {
        pump(10);
        plat_sleep_ms(1); /* the sync job runs on a real thread */
    }
    CHECK(g_saw_removing);
    wait_state(WYE_DRIVE_SYNCED);
    CHECK(g_drive.state == WYE_DRIVE_SYNCED && wye_disk_ready(&g_sim.target.disk));
    CHECK(exists_in_image("mine.txt"));
    /* A change there: it shows up here once the other computer is quiet. */
    wye_write_file_atomic(ROOT "/other/hello.txt", "hello", 5);
    wye_write_file_atomic(ROOT "/other/mine.txt", "mine", 4);
    wye_write_file_atomic(ROOT "/other/theirs.txt", "theirs", 6);
    edit_there(ROOT "/other");
    for (int i = 0; i < 400 && !plat_path_exists(ROOT "/folder/theirs.txt"); ++i) {
        pump(20);
        plat_sleep_ms(1);
    }
    char *text = (char *)wye_read_file(ROOT "/folder/theirs.txt", NULL, 64);
    CHECK(text && strcmp(text, "theirs") == 0);
    free(text);
    wye_drive_shutdown(&g_drive);
    tree_remove(ROOT);
}
