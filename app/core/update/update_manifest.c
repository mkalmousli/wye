/* Downloading and verifying the signed release manifest. */
#include "update.h"

#include "monocypher-ed25519.h"
#include "plat_sys.h"
#include "product.h"
#include "util/json.h"
#include "util/util.h"
#include "util/version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MANIFEST_MAX (256u * 1024u)

/* Generated at build time from keys/release.pub (hex). */
#ifndef WYE_RELEASE_PUBKEY_HEX
#define WYE_RELEASE_PUBKEY_HEX ""
#endif

static const uint8_t *release_pubkey(void)
{
    static uint8_t key[32];
    return wye_hex_decode(WYE_RELEASE_PUBKEY_HEX, key, 32) == 32 ? key : NULL;
}

static bool fetch(const char *url, const char *name, char *path, size_t cap, char *err, size_t errlen)
{
    char dir[PLAT_PATH_MAX];
    return plat_temp_dir(dir, sizeof(dir)) && wye_path_join(path, cap, dir, name) &&
           plat_http_download(url, path, MANIFEST_MAX, err, errlen);
}

static bool verify(const uint8_t *data, size_t len, const char *sig_hex)
{
    uint8_t sig[64];
    const uint8_t *pub = release_pubkey();
    char clean[160];
    wye_strcpy(clean, sizeof(clean), sig_hex);
    return pub && wye_hex_decode(wye_trim(clean), sig, sizeof(sig)) == 64 &&
           crypto_ed25519_check(sig, pub, data, len) == 0;
}

static bool parse(const char *json, struct wye_update_manifest *m)
{
    char path[96];
    memset(m, 0, sizeof(*m));
    snprintf(path, sizeof(path), "assets.%s.", plat_os_id());
    size_t base = strlen(path);
    bool ok = wye_json_string(json, "version", m->version, sizeof(m->version));
    wye_json_string(json, "notes", m->notes, sizeof(m->notes));
    wye_strcpy(path + base, sizeof(path) - base, "url");
    ok = ok && wye_json_string(json, path, m->url, sizeof(m->url));
    wye_strcpy(path + base, sizeof(path) - base, "sha512");
    ok = ok && wye_json_string(json, path, m->sha512, sizeof(m->sha512));
    wye_strcpy(path + base, sizeof(path) - base, "size");
    return ok && wye_json_number(json, path, &m->size) && wye_starts_with(m->url, "https://");
}

bool wye_update_fetch_manifest(const char *url, struct wye_update_manifest *m, char *err, size_t errlen)
{
    char json_path[PLAT_PATH_MAX], sig_path[PLAT_PATH_MAX], sig_url[600];
    snprintf(sig_url, sizeof(sig_url), "%s.sig", url);
    if (!fetch(url, "latest.json", json_path, sizeof(json_path), err, errlen) ||
        !fetch(sig_url, "latest.json.sig", sig_path, sizeof(sig_path), err, errlen)) {
        return false;
    }
    size_t len = 0;
    char *json = (char *)wye_read_file(json_path, &len, MANIFEST_MAX);
    char *sig = (char *)wye_read_file(sig_path, NULL, 1024);
    bool ok = json && sig && verify((const uint8_t *)json, len, sig);
    if (!ok) {
        wye_strcpy(err, errlen, "update manifest signature is invalid");
    } else if (!(ok = parse(json, m))) {
        wye_strcpy(err, errlen, "no update published for this system");
    }
    free(json);
    free(sig);
    return ok;
}
