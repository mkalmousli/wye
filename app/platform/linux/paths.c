/* XDG directories and system identification. */
#define _DEFAULT_SOURCE
#include "plat_sys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/utsname.h>
#include <unistd.h>

static bool xdg_dir(const char *env, const char *fallback, char *out, size_t len)
{
    const char *base = getenv(env);
    const char *home = getenv("HOME");
    int n;
    if (base && *base == '/') {
        n = snprintf(out, len, "%s/wye", base);
    } else {
        n = snprintf(out, len, "%s/%s/wye", home ? home : "/tmp", fallback);
    }
    return n > 0 && (size_t)n < len && plat_mkdirs(out);
}

bool plat_config_dir(char *out, size_t len)
{
    return xdg_dir("XDG_CONFIG_HOME", ".config", out, len);
}

bool plat_data_dir(char *out, size_t len)
{
    return xdg_dir("XDG_DATA_HOME", ".local/share", out, len);
}

bool plat_exe_path(char *out, size_t len)
{
    const char *appimage = getenv("APPIMAGE");
    if (appimage && *appimage) {
        snprintf(out, len, "%s", appimage);
        return true;
    }
    ssize_t n = readlink("/proc/self/exe", out, len - 1);
    if (n <= 0) {
        return false;
    }
    out[n] = '\0';
    return true;
}

const char *plat_os_id(void)
{
#if defined(__aarch64__)
    return "linux-arm64";
#else
    return "linux-x64";
#endif
}

void plat_os_description(char *out, size_t len)
{
    struct utsname u;
    char pretty[128] = "Linux";
    FILE *f = fopen("/etc/os-release", "r");
    char line[256];
    while (f && fgets(line, sizeof(line), f)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            const char *v = line + 12 + (line[12] == '"');
            snprintf(pretty, sizeof(pretty), "%.120s", v);
            pretty[strcspn(pretty, "\"\n")] = '\0';
        }
    }
    if (f) {
        fclose(f);
    }
    uname(&u);
    snprintf(out, len, "%s (kernel %s, %s, %s)", pretty, u.release, u.machine,
             getenv("WAYLAND_DISPLAY") ? "Wayland" : "X11");
}


bool plat_random(void *buf, size_t len)
{
    return getrandom(buf, len, 0) == (ssize_t)len;
}
