/* Where the shared drive lives on this computer. */
#include "drive.h"
#include "util/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ~/Wye Drive (Windows: %USERPROFILE%\Wye Drive). */
void drive_default_folder(char *out, size_t len)
{
    const char *home = getenv("HOME");
#if defined(_WIN32)
    home = getenv("USERPROFILE");
#endif
    snprintf(out, len, "%s%sWye Drive", home ? home : ".", plat_path_sep());
}

bool drive_paths(struct wye_drive *d)
{
    char data[PLAT_PATH_MAX];
    return plat_data_dir(data, sizeof(data)) && wye_path_join(d->image, sizeof(d->image), data, "drive.img") &&
           wye_path_join(d->manifest, sizeof(d->manifest), data, "drive-manifest.txt") &&
           wye_path_join(d->staging, sizeof(d->staging), data, "drive-staging");
}
