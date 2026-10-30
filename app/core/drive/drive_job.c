/* Syncing and packing the shared folder on a worker thread. */
#include "drive.h"
#include "drive_work.h"
#include "plat_thread.h"
#include "util/util.h"
#include <string.h>

static struct {
    plat_thread *thread;
    volatile bool done;
    bool pack;
    struct drive_work w;
} g;

static void run(void *arg)
{
    (void)arg;
    g.pack ? drive_work_pack(&g.w) : drive_work_sync(&g.w);
    g.done = true;
}

static bool launch(const struct wye_drive *d, bool pack_it, bool read_image)
{
    if (g.thread) {
        return false;
    }
    memset(&g.w, 0, sizeof(g.w));
    g.done = false;
    g.pack = pack_it;
    g.w.read_image = read_image;
    g.w.sectors = d->size_mb * 2048u;
    wye_strcpy(g.w.folder, sizeof(g.w.folder), d->folder);
    wye_strcpy(g.w.image, sizeof(g.w.image), d->image);
    wye_strcpy(g.w.manifest, sizeof(g.w.manifest), d->manifest);
    wye_strcpy(g.w.staging, sizeof(g.w.staging), d->staging);
    g.thread = plat_thread_start(run, NULL);
    return g.thread != NULL;
}

bool drive_job_sync(const struct wye_drive *d, bool read_image) { return launch(d, false, read_image); }
bool drive_job_pack(const struct wye_drive *d) { return launch(d, true, false); }

bool drive_job_poll(struct drive_job_result *out)
{
    if (!g.thread || !g.done) {
        return false;
    }
    drive_job_wait();
    *out = g.w.r;
    return true;
}

void drive_job_wait(void)
{
    plat_thread_join(g.thread);
    g.thread = NULL;
}
