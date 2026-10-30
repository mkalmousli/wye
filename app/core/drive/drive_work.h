/* Packing and syncing jobs (drive_work.c), run on the worker thread by drive_job.c. */
#ifndef WYE_DRIVE_WORK_H
#define WYE_DRIVE_WORK_H

#include "drive.h"

struct drive_work {
    bool read_image;               /* sync: unpack the image first (the other side changed it) */
    char folder[PLAT_PATH_MAX], image[PLAT_PATH_MAX], manifest[PLAT_PATH_MAX], staging[PLAT_PATH_MAX];
    uint32_t sectors;
    struct drive_job_result r;
};

void drive_work_pack(struct drive_work *w);
void drive_work_sync(struct drive_work *w);

#endif
