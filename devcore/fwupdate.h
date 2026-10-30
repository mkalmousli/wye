/*
 * Device-side firmware update receiver, shared by bridge and target.
 *
 * The image streams into a flash staging area in order, is verified by CRC32
 * (over the stream and again by reading flash back) and sanity-checked.
 * Installation is done by the firmware's RAM-resident copy routine.
 * Retransmitted chunks are acknowledged idempotently.
 */
#ifndef WYE_FWUPDATE_H
#define WYE_FWUPDATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WYE_FW_SECTOR 4096u

struct wye_flash_ops {
    void *ctx;
    uint32_t staging_size;
    /* Erase and program one sector at a staging-relative, sector-aligned offset. */
    bool (*write_sector)(void *ctx, uint32_t offset, const uint8_t *data);
    bool (*read)(void *ctx, uint32_t offset, uint8_t *out, size_t len);
    /* Board-specific image checks (vector table etc.); may be NULL. */
    bool (*validate)(void *ctx, uint32_t size);
};

struct wye_fwupdate {
    bool active;
    bool ready;            /* verified, waiting to be installed */
    uint32_t size;
    uint32_t crc;
    uint32_t next;
    uint32_t crc_running;
    uint8_t sector[WYE_FW_SECTOR];
};

void wye_fwupdate_reset(struct wye_fwupdate *fw);
/* All return WYE_FW_* status codes. */
uint8_t wye_fwupdate_begin(struct wye_fwupdate *fw, const struct wye_flash_ops *ops, uint32_t size,
                           uint32_t crc);
uint8_t wye_fwupdate_data(struct wye_fwupdate *fw, const struct wye_flash_ops *ops, uint32_t offset,
                          const uint8_t *data, size_t len, uint32_t *next_offset);
uint8_t wye_fwupdate_commit(struct wye_fwupdate *fw, const struct wye_flash_ops *ops);

/* Internal: write the partially filled sector ending at byte `upto`. */
bool wye_fwupdate_flush(struct wye_fwupdate *fw, const struct wye_flash_ops *ops, uint32_t upto);

#endif
