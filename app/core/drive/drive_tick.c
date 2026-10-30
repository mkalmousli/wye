/* The shared folder over time: insert, sync, take out briefly to update, put back. */
#include "drive.h"
#include <time.h>

#define RESEND_MS 1000u
#define IDLE_MS 800u        /* the other computer must be this quiet before its changes are read */
#define RETRY_MS 15000u

static void job_done(struct wye_drive *d, uint64_t now)
{
    struct drive_job_result r;
    if (!drive_job_poll(&r)) {
        return;
    }
    if (!r.ok) {
        drive_say(d, true, "%s", r.err);
    }
    if (d->state == WYE_DRIVE_PACKING) {
        d->state = r.ok && drive_open_image(d) ? WYE_DRIVE_INSERTING : WYE_DRIVE_OFF;
        d->set_sent = 0;
        d->next_check = now + (r.ok ? WYE_DRIVE_CHECK_MS : RETRY_MS);
    } else if (r.ok && r.need_pack) {
        d->state = WYE_DRIVE_REMOVING;
        d->set_sent = 0;
        drive_say(d, false, "Sending your changes to the other computer...");
    } else {
        d->state = WYE_DRIVE_SYNCED;
        d->next_check = now + WYE_DRIVE_CHECK_MS;
        if (r.ok) {
            d->synced_at = (int64_t)time(NULL);
            drive_say(d, false, "In sync");
        }
    }
}

static void synced(struct wye_drive *d, uint64_t now, bool target_ok)
{
    bool quiet = !(d->flags & WYE_DISK_BUSY) && now - d->last_write_ms >= IDLE_MS;
    bool there = d->dirty_there || d->writes != d->writes_synced;
    if ((there && quiet && target_ok) || now >= d->next_check) {
        bool read = there && quiet;
        if (drive_job_sync(d, read)) {
            d->state = WYE_DRIVE_SYNCING;
            if (read) {
                d->writes_synced = d->writes;
                d->dirty_there = false;
            }
        }
    }
}

void drive_tick_states(struct wye_drive *d, uint64_t now, bool target_ok, const struct wye_sender *out)
{
    bool active = d->state == WYE_DRIVE_INSERTING || d->state == WYE_DRIVE_SYNCED;
    if (active && (!d->enabled || d->restart)) {
        d->state = WYE_DRIVE_REMOVING;
        d->set_sent = 0;
    }
    switch (d->state) {
    case WYE_DRIVE_OFF:
        if (d->enabled && target_ok && now >= d->next_check) {
            drive_step_start(d, now);
        }
        break;
    case WYE_DRIVE_PACKING:
    case WYE_DRIVE_SYNCING:
        job_done(d, now);
        break;
    case WYE_DRIVE_INSERTING:
        if (target_ok && (d->flags & WYE_DISK_PRESENT)) {
            d->state = WYE_DRIVE_SYNCED;
            drive_say(d, false, "In sync");
        } else if (target_ok && (!d->set_sent || now - d->set_sent > RESEND_MS)) {
            wye_drive_send_set(d, true, now, out);
        }
        break;
    case WYE_DRIVE_SYNCED:
        synced(d, now, target_ok);
        break;
    case WYE_DRIVE_REMOVING:
        drive_step_removing(d, now, target_ok, out);
        break;
    }
}
