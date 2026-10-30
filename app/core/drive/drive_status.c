/* The shared folder: status for the UI, and "sync now". */
#include "drive.h"
#include "util/util.h"
#include <string.h>

void wye_drive_sync_now(struct wye_drive *d)
{
    d->next_check = 0;
    d->dirty_there = true;
}

void wye_drive_status(const struct wye_drive *d, struct wye_drive_status *out)
{
    memset(out, 0, sizeof(*out));
    out->state = d->state;
    out->enabled = d->enabled;
    out->capable = d->capable;
    out->busy_there = (d->flags & WYE_DISK_BUSY) != 0;
    out->error = d->error;
    wye_strcpy(out->message, sizeof(out->message), d->message);
    wye_strcpy(out->folder, sizeof(out->folder), d->folder);
    out->size_mb = d->size_mb;
    out->reads = d->reads;
    out->writes = d->writes;
    out->synced_at = d->synced_at;
}
