/*
 * Target side of the shared drive. The target PC sees a USB drive; every
 * block it reads or writes is fetched from / stored on the host over the
 * link (a few blocks are read ahead). The host decides whether the medium is
 * inserted, so the drive can move between the two computers safely.
 *
 * wye_disk_read/write are called from the USB mass-storage callbacks and
 * return WYE_DISK_BLOCK when done, 0 while waiting (keep pumping the UART)
 * or -1 on failure (no medium, out of range, host not answering).
 */
#ifndef WYE_TARGET_DISK_H
#define WYE_TARGET_DISK_H

#include "wye_codec.h"
#include <stdbool.h>
#include <stdint.h>

#define WYE_DISK_RETRY_MS 400u
#define WYE_DISK_TIMEOUT_MS 4000u
#define WYE_DISK_IDLE_MS 1500u   /* after the last write, removing the medium is safe */

struct wye_target;

struct wye_disk {
    bool present, ejected, changed;    /* changed: report "medium changed" once */
    uint32_t blocks;
    uint8_t reported;                  /* last WYE_DISK_* flags sent to the host */
    /* read-ahead cache: [cache_lba, cache_lba + cache_count) */
    bool reading;
    uint32_t cache_lba, read_sent, read_started;
    uint8_t cache_count;
    uint16_t got[WYE_DISK_READAHEAD];
    uint8_t cache[WYE_DISK_READAHEAD][WYE_DISK_BLOCK];
    /* one write in flight */
    bool writing, write_acked, write_failed, wrote;
    uint32_t write_lba, write_sent, write_started, last_write;
    uint8_t wbuf[WYE_DISK_BLOCK];
};

void wye_disk_init(struct wye_disk *d);
bool wye_disk_ready(const struct wye_disk *d);
int32_t wye_disk_read(struct wye_target *t, uint32_t lba, uint8_t *out);
int32_t wye_disk_write(struct wye_target *t, uint32_t lba, const uint8_t *data);
/* The target PC ejected the drive (SCSI START STOP UNIT). */
void wye_disk_eject(struct wye_target *t);
/* Host messages (DISK_SET, DISK_DATA, DISK_WACK); true if consumed. */
bool wye_disk_handle(struct wye_target *t, const struct wye_packet *pkt);
/* Reports flag changes to the host; call from the main loop. */
void wye_disk_task(struct wye_target *t);
uint8_t wye_disk_flags(const struct wye_target *t);

#endif
