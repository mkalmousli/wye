/* Shared-folder sync decisions (drive_sync.c) and the job that runs them (drive_job.c). */
#ifndef WYE_DRIVE_SYNC_H
#define WYE_DRIVE_SYNC_H

#include "sync_list.h"

struct sync_dirs {
    const char *folder;        /* the shared folder here */
    const char *staging;       /* the image unpacked (other computer's view) */
};

/*
 * Applies the other computer's changes (staged; NULL when the image is
 * unchanged) to the folder. Returns true when the folder has changes the
 * image must get, i.e. the image needs repacking.
 */
bool drive_sync_apply(const struct sync_dirs *d, const struct sync_list *staged, const struct sync_list *manifest);

#endif
