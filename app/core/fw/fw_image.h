/*
 * Firmware images embedded in the app binary at build time, and UF2
 * conversion. The app always carries firmware matching its own version.
 */
#ifndef WYE_FW_IMAGE_H
#define WYE_FW_IMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WYE_UF2_FAMILY_RP2040 0xe48bff56u
#define WYE_FLASH_BASE 0x10000000u

struct wye_fw_image {
    uint8_t role;              /* WYE_ROLE_* */
    const char *name;          /* "bridge" / "target" */
    const uint8_t *uf2;
    size_t uf2_len;
    uint8_t major, minor, patch;
};

/* NULL if this build carries no firmware for the role. */
const struct wye_fw_image *wye_fw_embedded(uint8_t role);

/* Flattens a UF2 into a flash image starting at WYE_FLASH_BASE (malloc'd). */
bool wye_uf2_to_bin(const uint8_t *uf2, size_t len, uint8_t **bin, size_t *bin_len);

/* True if (major, minor, patch) is older than the embedded image. */
bool wye_fw_is_older(const struct wye_fw_image *img, uint8_t major, uint8_t minor, uint8_t patch);

#endif
