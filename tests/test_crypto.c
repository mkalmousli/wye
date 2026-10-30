/* Manifest JSON, version ordering, update hashing. */
#include "test.h"
#include "update/update.h"
#include "util/json.h"
#include "util/util.h"
#include "util/version.h"

#include <stdio.h>

TEST(json_paths)
{
    const char *j = "{\"version\":\"3.1.0\",\"notes\":\"a \\\"b\\\"\",\"assets\":{\"x\":{\"n\":[1,{\"q\":2}]},"
                    "\"linux-x64\":{\"url\":\"https://e/x\",\"size\":1234}}}";
    char s[64];
    double d;
    CHECK(wye_json_string(j, "version", s, sizeof(s)));
    CHECK(strcmp(s, "3.1.0") == 0);
    CHECK(wye_json_string(j, "notes", s, sizeof(s)));
    CHECK(strcmp(s, "a \"b\"") == 0);
    CHECK(wye_json_string(j, "assets.linux-x64.url", s, sizeof(s)));
    CHECK(wye_json_number(j, "assets.linux-x64.size", &d) && d == 1234);
    CHECK(wye_json_find(j, "assets.missing") == NULL);
    CHECK(wye_json_find("{\"a\":", "a.b") == NULL);
}

TEST(version_ordering)
{
    CHECK(wye_version_compare("3.0.10", "3.0.9") > 0);
    CHECK(wye_version_compare("v3.1.0", "3.1.0") == 0);
    CHECK(wye_version_compare("3.1.0-beta", "3.1.0") < 0);
    CHECK(wye_version_compare("2.9.9", "3.0.0") < 0);
}

TEST(update_sha512_verification)
{
    const char *path = "wye_test_sha.bin";
    FILE *f = fopen(path, "wb");
    fputs("abc", f);
    fclose(f);
    const char *abc = "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                      "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f";
    CHECK(wye_update_verify_sha512(path, abc, 3));
    CHECK(!wye_update_verify_sha512(path, abc, 4));
    remove(path);
}
