/* The shared folder: setup, settings and status (engine thread, engine lock held). */
#include "drive.h"
#include "log.h"
#include "util/util.h"
#include <stdarg.h>
#include <string.h>

void drive_say(struct wye_drive *d, bool error, const char *fmt, ...)
{
    char was[sizeof(d->message)];
    va_list ap;
    wye_strcpy(was, sizeof(was), d->message);
    va_start(ap, fmt);
    vsnprintf(d->message, sizeof(d->message), fmt, ap);
    va_end(ap);
    d->error = error;
    if (strcmp(was, d->message) != 0) { /* "In sync" every half second is not news */
        LOG_I("drive: %s", d->message);
    }
}

bool drive_open_image(struct wye_drive *d)
{
    drive_close_image(d);
    d->img = fopen(d->image, "r+b");
    d->blocks = 0;
    if (d->img && fseek(d->img, 0, SEEK_END) == 0) {
        d->blocks = (uint32_t)(wye_ftell64(d->img) / WYE_DISK_BLOCK);
    }
    return d->img && d->blocks > 0;
}

void drive_close_image(struct wye_drive *d)
{
    if (d->img) {
        fclose(d->img);
        d->img = NULL;
    }
}

void wye_drive_init(struct wye_drive *d, bool enabled, const char *folder, uint32_t size_mb)
{
    memset(d, 0, sizeof(*d));
    drive_paths(d);
    wye_drive_configure(d, enabled, folder, size_mb);
    d->restart = false;
    d->dirty_there = plat_path_exists(d->image); /* it may hold changes not copied here yet */
    drive_say(d, false, enabled ? "Waiting for the other computer" : "The shared folder is off");
}

void wye_drive_configure(struct wye_drive *d, bool enabled, const char *folder, uint32_t size_mb)
{
    char want[PLAT_PATH_MAX];
    if (folder && folder[0]) {
        wye_strcpy(want, sizeof(want), folder);
    } else {
        drive_default_folder(want, sizeof(want));
    }
    if (strcmp(want, d->folder) != 0 || size_mb != d->size_mb) {
        bool first = d->folder[0] == '\0';
        wye_strcpy(d->folder, sizeof(d->folder), want);
        d->size_mb = size_mb;
        if (!first) { /* rebuilt from the new folder / size */
            d->restart = d->state != WYE_DRIVE_OFF;
            remove(d->manifest);
        }
    }
    d->enabled = enabled;
    plat_mkdirs(d->folder);
    if (enabled) {
        plat_places_pin(d->folder, "Wye Drive"); /* here it shows up like the drive does there */
    }
}

void wye_drive_shutdown(struct wye_drive *d)
{
    drive_job_wait();
    drive_close_image(d);
}

void wye_drive_tick(struct wye_drive *d, uint64_t now, bool target_ready, bool capable, const struct wye_sender *out)
{
    d->capable = capable;
    if (!target_ready) {
        d->flags = 0; /* the target forgets the medium when it restarts */
    }
    if (d->writes != d->writes_seen) { /* the other computer wrote something */
        d->writes_seen = d->writes;
        d->last_write_ms = now;
    }
    drive_tick_states(d, now, target_ready && capable, out);
}
