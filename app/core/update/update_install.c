/* Downloading, verifying and installing an update package. */
#include "update.h"

#include "log.h"
#include "monocypher-ed25519.h"
#include "plat_sys.h"
#include "util/util.h"

#include <stdio.h>
#include <string.h>

bool wye_update_verify_sha512(const char *path, const char *hex, double size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    crypto_sha512_ctx ctx;
    crypto_sha512_init(&ctx);
    uint8_t buf[65536], digest[64];
    size_t n, total = 0;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        crypto_sha512_update(&ctx, buf, n);
        total += n;
    }
    fclose(f);
    crypto_sha512_final(&ctx, digest);
    char got[129];
    wye_hex_encode(digest, sizeof(digest), got, sizeof(got));
    return (double)total == size && strlen(hex) == 128 && wye_contains_ci(got, hex);
}

bool wye_update_install(const struct wye_update_manifest *m, char *err, size_t errlen)
{
    char dir[PLAT_PATH_MAX], path[PLAT_PATH_MAX];
    if (!plat_temp_dir(dir, sizeof(dir)) || !wye_path_join(path, sizeof(path), dir, "update.pkg")) {
        wye_strcpy(err, errlen, "no temporary directory");
        return false;
    }
    LOG_I("update: downloading %s", m->url);
    if (!plat_http_download(m->url, path, WYE_UPDATE_MAX_ASSET, err, errlen)) {
        return false;
    }
    if (!wye_update_verify_sha512(path, m->sha512, m->size)) {
        remove(path);
        wye_strcpy(err, errlen, "downloaded update failed verification");
        return false;
    }
    LOG_I("update: verified %s, installing", m->version);
    bool ok = plat_install_update(path, err, errlen);
    remove(path);
    return ok;
}
