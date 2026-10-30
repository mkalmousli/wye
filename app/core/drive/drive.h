/*
 * The shared folder, host side. One folder that both computers work in:
 * the other computer sees it as a USB drive (WYE) whose blocks this app
 * serves from a FAT image through the bridge; the image and the folder here
 * are kept in sync automatically (drive_sync.c).
 *
 *   Changes there: under a second after the other computer stops writing, the
 *     image is unpacked and the changes are copied into the folder.
 *   Changes here: once the other computer is idle the drive is taken out for
 *     a moment, the image is rebuilt from the folder and put back.
 */
#ifndef WYE_DRIVE_H
#define WYE_DRIVE_H

#include "features/sender.h"
#include "wye_codec.h"
#include "wye_msg.h"
#include "plat_sys.h"
#include <stdio.h>

#define WYE_DRIVE_CHECK_MS 500u  /* how often the folder here is checked for changes (a quick scan) */

enum wye_drive_state {
    WYE_DRIVE_OFF = 0,       /* switched off, or no target with the drive */
    WYE_DRIVE_PACKING,       /* building the image from the folder */
    WYE_DRIVE_INSERTING,     /* waiting for the other computer to see the drive */
    WYE_DRIVE_SYNCED,
    WYE_DRIVE_SYNCING,       /* comparing and copying changes */
    WYE_DRIVE_REMOVING,      /* taking the drive out briefly to update it */
};

struct wye_drive_status {
    enum wye_drive_state state;
    bool enabled, capable, busy_there, error;
    char message[160];
    char folder[PLAT_PATH_MAX];
    uint32_t size_mb;
    unsigned long reads, writes;
    int64_t synced_at;       /* unix time of the last completed sync, 0 = never */
};

struct wye_drive {
    enum wye_drive_state state;
    bool enabled, capable, error, restart, dirty_there;
    char folder[PLAT_PATH_MAX], image[PLAT_PATH_MAX], manifest[PLAT_PATH_MAX], staging[PLAT_PATH_MAX];
    uint32_t size_mb, blocks;
    FILE *img;               /* open while served to the target */
    uint8_t flags;           /* last WYE_DISK_* flags from the target */
    uint64_t set_sent, next_check, last_write_ms;
    unsigned long writes_seen, writes_synced;
    uint32_t wlba;
    uint16_t wgot;
    uint8_t wbuf[WYE_DISK_BLOCK];
    char message[160];
    unsigned long reads, writes;
    int64_t synced_at;
};

void wye_drive_init(struct wye_drive *d, bool enabled, const char *folder, uint32_t size_mb);
void wye_drive_configure(struct wye_drive *d, bool enabled, const char *folder, uint32_t size_mb);
void wye_drive_sync_now(struct wye_drive *d);
void wye_drive_tick(struct wye_drive *d, uint64_t now, bool target_ready, bool capable, const struct wye_sender *out);
void wye_drive_status(const struct wye_drive *d, struct wye_drive_status *out);
void wye_drive_shutdown(struct wye_drive *d);

/* drive_server.c: serving blocks to the target */
void wye_drive_packet(struct wye_drive *d, const struct wye_packet *pkt, const struct wye_sender *out);
void wye_drive_send_set(struct wye_drive *d, bool present, uint64_t now, const struct wye_sender *out);

/* Internal (drive.c, drive_tick.c) */
void drive_say(struct wye_drive *d, bool error, const char *fmt, ...);
bool drive_open_image(struct wye_drive *d);
void drive_close_image(struct wye_drive *d);
void drive_tick_states(struct wye_drive *d, uint64_t now, bool target_ok, const struct wye_sender *out);
void drive_step_start(struct wye_drive *d, uint64_t now);
void drive_step_removing(struct wye_drive *d, uint64_t now, bool target_ok, const struct wye_sender *out);

/* drive_job.c: syncing and packing on a worker thread (the engine thread stays responsive) */
struct drive_job_result {
    bool ok, need_pack;
    char err[160];
};
bool drive_job_sync(const struct wye_drive *d, bool read_image);
bool drive_job_pack(const struct wye_drive *d);
bool drive_job_poll(struct drive_job_result *out);   /* true once finished */
void drive_job_wait(void);

/* drive_paths.c */
void drive_default_folder(char *out, size_t len);
bool drive_paths(struct wye_drive *d);

#endif
