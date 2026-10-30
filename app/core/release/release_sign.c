#include "release_sign.h"

#include "monocypher-ed25519.h"
#include "monocypher.h"
#include "plat_sys.h"
#include "util/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_MAX (64u * 1024u * 1024u)

static bool write_hex(const char *path, const uint8_t *data, size_t len)
{
    char hex[257];
    wye_hex_encode(data, len, hex, sizeof(hex));
    wye_strcat(hex, sizeof(hex), "\n");
    return wye_write_file_atomic(path, hex, strlen(hex));
}

bool wye_release_keygen(const char *dir, const char *name, char *err, size_t errlen)
{
    char key_path[PLAT_PATH_MAX], pub_path[PLAT_PATH_MAX], file[96];
    snprintf(file, sizeof(file), "%s.key", name);
    wye_path_join(key_path, sizeof(key_path), dir, file);
    snprintf(file, sizeof(file), "%s.pub", name);
    wye_path_join(pub_path, sizeof(pub_path), dir, file);
    if (plat_path_exists(key_path)) {
        snprintf(err, errlen, "%s already exists - refusing to overwrite a key", key_path);
        return false;
    }
    uint8_t seed[32], secret[64], pub[32];
    if (!plat_random(seed, sizeof(seed))) {
        snprintf(err, errlen, "no secure random source");
        return false;
    }
    crypto_ed25519_key_pair(secret, pub, seed); /* wipes seed */
    bool ok = write_hex(key_path, secret, 64) && write_hex(pub_path, pub, 32);
    wye_release_wipe(secret);
    if (!ok) {
        snprintf(err, errlen, "cannot write key files in %s", dir);
    }
    return ok;
}

bool wye_release_load_secret(const char *path, uint8_t secret[64])
{
    char *text = (char *)wye_read_file(path, NULL, 1024);
    bool ok = text && wye_hex_decode(wye_trim(text), secret, 64) == 64;
    if (text) {
        crypto_wipe(text, strlen(text));
        free(text);
    }
    return ok;
}

bool wye_release_sign_file(const uint8_t secret[64], const char *path, char *err, size_t errlen)
{
    size_t len = 0;
    uint8_t *data = wye_read_file(path, &len, FILE_MAX);
    if (!data) {
        snprintf(err, errlen, "cannot read %s", path);
        return false;
    }
    uint8_t sig[64];
    crypto_ed25519_sign(sig, secret, data, len);
    free(data);
    char sig_path[PLAT_PATH_MAX];
    snprintf(sig_path, sizeof(sig_path), "%s.sig", path);
    if (!write_hex(sig_path, sig, sizeof(sig))) {
        snprintf(err, errlen, "cannot write %s", sig_path);
        return false;
    }
    return true;
}

void wye_release_wipe(uint8_t secret[64])
{
    crypto_wipe(secret, 64);
}
