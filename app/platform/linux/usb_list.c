/* USB devices attached to this computer, from sysfs (root hubs left out). */
#define _DEFAULT_SOURCE
#include "plat_sys.h"
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void read_str(const char *dir, const char *name, char *out, size_t len)
{
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "r");
    out[0] = '\0';
    if (f) {
        if (fgets(out, (int)len, f)) {
            out[strcspn(out, "\r\n")] = '\0';
        }
        fclose(f);
    }
}

int plat_usb_list(struct plat_usb_device *out, int max)
{
    DIR *d = opendir("/sys/bus/usb/devices");
    int n = 0;
    for (struct dirent *e = d ? readdir(d) : NULL; e && n < max; e = readdir(d)) {
        if (e->d_name[0] == '.' || strchr(e->d_name, ':') || strncmp(e->d_name, "usb", 3) == 0) {
            continue; /* interfaces and root hubs */
        }
        char dir[PATH_MAX], hex[16];
        snprintf(dir, sizeof(dir), "/sys/bus/usb/devices/%s", e->d_name);
        struct plat_usb_device *u = &out[n];
        read_str(dir, "idVendor", hex, sizeof(hex));
        u->vid = (uint16_t)strtoul(hex, NULL, 16);
        read_str(dir, "idProduct", hex, sizeof(hex));
        u->pid = (uint16_t)strtoul(hex, NULL, 16);
        read_str(dir, "manufacturer", u->manufacturer, sizeof(u->manufacturer));
        read_str(dir, "product", u->product, sizeof(u->product));
        read_str(dir, "serial", u->serial, sizeof(u->serial));
        n += u->vid != 0;
    }
    if (d) {
        closedir(d);
    }
    return n;
}
