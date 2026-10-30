/* Opening URLs/folders and start-at-login via XDG autostart. */
#define _DEFAULT_SOURCE
#include "plat_sys.h"
#include "proc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool plat_open_url(const char *url)
{
    const char *argv[] = {"xdg-open", url, NULL};
    return posix_spawn_detached(argv);
}

bool plat_reveal_path(const char *path)
{
    return plat_open_url(path);
}

static bool autostart_file(char *out, size_t len)
{
    const char *base = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    char dir[PLAT_PATH_MAX];
    if (base && *base == '/') {
        snprintf(dir, sizeof(dir), "%s/autostart", base);
    } else {
        snprintf(dir, sizeof(dir), "%s/.config/autostart", home ? home : "/tmp");
    }
    int n = snprintf(out, len, "%s/wye.desktop", dir);
    return n > 0 && (size_t)n < len && plat_mkdirs(dir);
}

bool plat_autostart_get(void)
{
    char path[PLAT_PATH_MAX];
    return autostart_file(path, sizeof(path)) && access(path, F_OK) == 0;
}

bool plat_autostart_set(bool enable)
{
    char path[PLAT_PATH_MAX], exe[PLAT_PATH_MAX];
    if (!autostart_file(path, sizeof(path))) {
        return false;
    }
    if (!enable) {
        return remove(path) == 0 || !plat_path_exists(path);
    }
    if (!plat_exe_path(exe, sizeof(exe))) {
        return false;
    }
    FILE *f = fopen(path, "w");
    if (!f) {
        return false;
    }
    fprintf(f, "[Desktop Entry]\nType=Application\nName=Wye\nExec=\"%s\" --background\n"
               "Icon=wye\nX-GNOME-Autostart-enabled=true\nTerminal=false\n", exe);
    return fclose(f) == 0;
}
