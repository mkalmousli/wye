/* Enumerates /dev/ttyACM* with USB identity read from sysfs. */
#define _DEFAULT_SOURCE
#include "plat_serial.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool read_attr(const char *dir, const char *name, char *out, size_t len)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "r");
    if (!f) {
        return false;
    }
    bool ok = fgets(out, (int)len, f) != NULL;
    fclose(f);
    out[strcspn(out, "\n")] = '\0';
    return ok;
}

/* /sys/class/tty/ttyACM0/device -> interface dir; its parent is the USB device. */
static void fill_usb(const char *tty, struct plat_port_info *p)
{
    char dev[512], buf[64];
    snprintf(dev, sizeof(dev), "/sys/class/tty/%s/device/..", tty);
    if (read_attr(dev, "idVendor", buf, sizeof(buf))) {
        p->vid = (uint16_t)strtoul(buf, NULL, 16);
    }
    if (read_attr(dev, "idProduct", buf, sizeof(buf))) {
        p->pid = (uint16_t)strtoul(buf, NULL, 16);
    }
    read_attr(dev, "product", p->product, sizeof(p->product));
    read_attr(dev, "serial", p->serial, sizeof(p->serial));
}

int plat_serial_list(struct plat_port_info *out, int max)
{
    DIR *d = opendir("/sys/class/tty");
    if (!d) {
        return 0;
    }
    int n = 0;
    for (struct dirent *e = readdir(d); e && n < max; e = readdir(d)) {
        if (strncmp(e->d_name, "ttyACM", 6) != 0) {
            continue;
        }
        struct plat_port_info *p = &out[n];
        memset(p, 0, sizeof(*p));
        snprintf(p->path, sizeof(p->path), "/dev/%.200s", e->d_name);
        fill_usb(e->d_name, p);
        n++;
    }
    closedir(d);
    return n;
}
