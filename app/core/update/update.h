/*
 * Signed automatic updates.
 *
 * latest.json (+ latest.json.sig = hex Ed25519 signature of the exact bytes,
 * by the release key) is published with each GitHub release:
 *   {"version":"3.0.1","notes":"...","assets":{"<os-id>":
 *       {"url":"https://...","sha512":"<hex>","size":123}}}
 * The app verifies the signature, then the downloaded asset's size and
 * SHA-512, before replacing itself.
 */
#ifndef WYE_UPDATE_H
#define WYE_UPDATE_H

#include "config/config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WYE_UPDATE_MAX_ASSET (256u * 1024u * 1024u)
#define WYE_UPDATE_CHECK_INTERVAL_S (6 * 3600)

enum wye_update_state {
    WYE_UPD_IDLE = 0,
    WYE_UPD_CHECKING,
    WYE_UPD_CURRENT,       /* checked, nothing newer */
    WYE_UPD_AVAILABLE,
    WYE_UPD_DOWNLOADING,
    WYE_UPD_RESTARTING,    /* installed; the app should exit now */
    WYE_UPD_ERROR,
};

struct wye_update_manifest {
    char version[32];
    char notes[1024];
    char url[512];
    char sha512[129];
    double size;
};

struct wye_update_status {
    enum wye_update_state state;
    struct wye_update_manifest manifest;
    char error[160];
    int64_t last_check;
};

/* Fetches and verifies the manifest; true if it parsed and verified. */
bool wye_update_fetch_manifest(const char *url, struct wye_update_manifest *m, char *err, size_t errlen);
/* Downloads, verifies and installs; true means the app must exit to finish. */
bool wye_update_install(const struct wye_update_manifest *m, char *err, size_t errlen);
bool wye_update_verify_sha512(const char *path, const char *hex, double size);

/* Background service (update_service.c). */
void wye_updater_start(const struct wye_config *cfg);
void wye_updater_stop(void);
void wye_updater_configure(const struct wye_config *cfg);
void wye_updater_check_now(void);
void wye_updater_install_now(void);
void wye_updater_status(struct wye_update_status *out);
/* Install blockers (e.g. forwarding active); checked before auto-install. */
void wye_updater_set_busy(bool busy);

#endif
