/* Finds mounted UF2 bootloader drives (they contain INFO_UF2.TXT). */
#define _DEFAULT_SOURCE
#include "plat_sys.h"

#include <mntent.h>
#include <stdio.h>
#include <string.h>

int plat_uf2_drives(char paths[][PLAT_PATH_MAX], int max)
{
    FILE *m = setmntent("/proc/self/mounts", "r");
    if (!m) {
        return 0;
    }
    int n = 0;
    for (struct mntent *e = getmntent(m); e && n < max; e = getmntent(m)) {
        if (strcmp(e->mnt_type, "vfat") != 0 && strcmp(e->mnt_type, "fuseblk") != 0) {
            continue;
        }
        char info[PLAT_PATH_MAX + 16];
        snprintf(info, sizeof(info), "%s/INFO_UF2.TXT", e->mnt_dir);
        if (plat_path_exists(info)) {
            snprintf(paths[n++], PLAT_PATH_MAX, "%s", e->mnt_dir);
        }
    }
    endmntent(m);
    return n;
}
