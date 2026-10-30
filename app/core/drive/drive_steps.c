/* The shared folder: starting up, and taking the drive out to update it. */
#include "drive.h"

#define RESEND_MS 1000u

void drive_step_start(struct wye_drive *d, uint64_t now)
{
    /* Without a manifest an existing image is merged (first run after an update), never overwritten. */
    bool fresh = !plat_path_exists(d->image) || d->restart;
    if (fresh) {
        d->restart = false;
        if (drive_job_pack(d)) {
            d->state = WYE_DRIVE_PACKING;
            drive_say(d, false, "Preparing the shared folder...");
        }
    } else if (drive_open_image(d)) {
        d->state = WYE_DRIVE_INSERTING;
        d->set_sent = 0;
    } else {
        d->restart = true; /* unreadable image: rebuild it */
    }
    d->next_check = now + WYE_DRIVE_CHECK_MS;
}

void drive_step_removing(struct wye_drive *d, uint64_t now, bool target_ok, const struct wye_sender *out)
{
    if (target_ok && (d->flags & WYE_DISK_BUSY)) {
        drive_say(d, false, "Waiting for the other computer to finish writing...");
    } else if (target_ok && (d->flags & WYE_DISK_PRESENT)) {
        if (!d->set_sent || now - d->set_sent > RESEND_MS) {
            wye_drive_send_set(d, false, now, out);
        }
    } else { /* out (or the target is gone): nobody writes to the image now */
        drive_close_image(d);
        d->state = WYE_DRIVE_OFF;
        if (d->enabled && drive_job_pack(d)) {
            d->restart = false;
            d->state = WYE_DRIVE_PACKING;
        }
    }
}
