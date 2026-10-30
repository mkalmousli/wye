/*
 * Release signing tools, used by the release workflow and by anyone publishing
 * their own builds. Private keys never ship in the app; only the public half
 * of the release key is built in.
 *
 * Key files are hex text: "<name>.key" (64-byte secret), "<name>.pub" (32).
 */
#ifndef WYE_RELEASE_SIGN_H
#define WYE_RELEASE_SIGN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Writes <dir>/<name>.key and <dir>/<name>.pub; refuses to overwrite. */
bool wye_release_keygen(const char *dir, const char *name, char *err, size_t errlen);
bool wye_release_load_secret(const char *path, uint8_t secret[64]);
/* Writes "<path>.sig" (hex Ed25519 signature of the file's exact bytes). */
bool wye_release_sign_file(const uint8_t secret[64], const char *path, char *err, size_t errlen);
void wye_release_wipe(uint8_t secret[64]);

#endif
