/* Shared drive images: a folder packed into FAT16 and unpacked again comes back identical. */
#include "drive/fat_tree.h"
#include "plat_sys.h"
#include "test.h"

#include <stdlib.h>
#include <string.h>

#define ROOT "out/test-fat"

static void put(const char *path, size_t size, uint8_t seed)
{
    FILE *f = fopen(path, "wb");
    for (size_t i = 0; f && i < size; ++i) {
        fputc((int)((i * 7 + seed) & 0xff), f);
    }
    if (f) {
        fclose(f);
    }
}

static bool same(const char *a, const char *b)
{
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    int ca = 0, cb = 0;
    while (fa && fb && (ca = fgetc(fa)) == (cb = fgetc(fb)) && ca != EOF) {
    }
    bool ok = fa && fb && ca == EOF && cb == EOF;
    if (fa) fclose(fa);
    if (fb) fclose(fb);
    return ok;
}

TEST(fat_pack_unpack_round_trip)
{
    static const char *const files[] = {"short.txt", "A long file name with spaces.txt", "Gr\xc3\xbc\303\237e.md",
                                        "docs/inner/deep.bin", "docs/empty", "noext"};
    static const size_t sizes[] = {12, 5000, 3, 70000, 0, 1};
    char err[200] = "";
    tree_remove(ROOT);
    CHECK(plat_mkdirs(ROOT "/src/docs/inner") && plat_mkdirs(ROOT "/dst"));
    for (int i = 0; i < 6; ++i) {
        char *p = tree_join(ROOT "/src", files[i]);
        put(p, sizes[i], (uint8_t)i);
        free(p);
    }
    FILE *img = fopen(ROOT "/drive.img", "w+b");
    CHECK(img != NULL);
    CHECK(fat_pack(ROOT "/src", img, 32u * 2048u, err, sizeof(err)));
    CHECK(fat_unpack(img, ROOT "/dst", err, sizeof(err)));
    fclose(img);
    for (int i = 0; i < 6; ++i) {
        char *a = tree_join(ROOT "/src", files[i]), *b = tree_join(ROOT "/dst", files[i]);
        CHECK(same(a, b));
        free(a);
        free(b);
    }
    tree_remove(ROOT);
}

TEST(fat_reports_a_full_drive)
{
    char err[200] = "";
    tree_remove(ROOT);
    CHECK(plat_mkdirs(ROOT "/src"));
    put(ROOT "/src/big.bin", 5u * 1024u * 1024u, 1);
    FILE *img = fopen(ROOT "/drive.img", "w+b");
    CHECK(!fat_pack(ROOT "/src", img, 4u * 2048u, err, sizeof(err))); /* 5 MB file, 4 MB drive */
    CHECK(strstr(err, "Not enough space") != NULL);
    fclose(img);
    tree_remove(ROOT);
}

TEST(fat32_with_partition_table_round_trip)
{
    char err[200] = "";
    uint8_t mbr[512];
    tree_remove(ROOT);
    CHECK(plat_mkdirs(ROOT "/src/a/b") && plat_mkdirs(ROOT "/dst"));
    put(ROOT "/src/a/b/deep file.bin", 200000, 3);
    put(ROOT "/src/top.txt", 10, 4);
    FILE *img = fopen(ROOT "/drive.img", "w+b"); /* 80 MB: FAT32 (sparse, so it costs little) */
    CHECK(img && fat_pack(ROOT "/src", img, 80u * 2048u, err, sizeof(err)));
    CHECK(img && fseek(img, 0, SEEK_SET) == 0 && fread(mbr, 1, 512, img) == 512);
    CHECK(mbr[446 + 4] == 0x0c && mbr[510] == 0x55 && mbr[511] == 0xaa); /* FAT32 LBA partition */
    CHECK(img && fat_unpack(img, ROOT "/dst", err, sizeof(err)));
    if (img) {
        fclose(img);
    }
    CHECK(same(ROOT "/src/a/b/deep file.bin", ROOT "/dst/a/b/deep file.bin"));
    CHECK(same(ROOT "/src/top.txt", ROOT "/dst/top.txt"));
    tree_remove(ROOT);
}
