/* Picos in BOOTSEL mode: counted on USB via sysfs; their RPI-RP2/RP2350 drive is
   mounted through udisks (then gio) when the desktop does not automount it. */
#define _DEFAULT_SOURCE
#include "log.h"
#include "plat_sys.h"
#include "plat_thread.h"
#include "proc.h"
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOUNT_RETRY_MS 3000u
#define MOUNT_TRIES 4

static unsigned read_hex(const char *dir, const char *name)
{
    char path[PATH_MAX], buf[16] = "";
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "r");
    if (!f) {
        return 0;
    }
    bool ok = fgets(buf, sizeof(buf), f) != NULL;
    fclose(f);
    return ok ? (unsigned)strtoul(buf, NULL, 16) : 0;
}

int plat_usb_count(uint16_t vid, uint16_t pid)
{
    DIR *d = opendir("/sys/bus/usb/devices");
    int n = 0;
    for (struct dirent *e = d ? readdir(d) : NULL; e; e = readdir(d)) {
        if (e->d_name[0] == '.' || strchr(e->d_name, ':')) {
            continue; /* interfaces ("1-2:1.0") carry no idVendor */
        }
        char dir[PATH_MAX];
        snprintf(dir, sizeof(dir), "/sys/bus/usb/devices/%s", e->d_name);
        n += read_hex(dir, "idVendor") == vid && read_hex(dir, "idProduct") == pid;
    }
    if (d) {
        closedir(d);
    }
    return n;
}

static bool is_mounted(const char *dev)
{
    FILE *f = fopen("/proc/self/mounts", "r");
    char line[1024];
    bool found = false;
    size_t n = strlen(dev);
    while (f && !found && fgets(line, sizeof(line), f)) {
        found = strncmp(line, dev, n) == 0 && line[n] == ' ';
    }
    if (f) {
        fclose(f);
    }
    return found;
}

/* One mount attempt every few seconds, alternating udisksctl and gio. */
static void automount(struct plat_bootsel *out)
{
    static const char *const LABELS[] = {"RPI-RP2", "RP2350"};
    static struct { char dev[PATH_MAX]; int tries; uint64_t next; } g;
    for (int i = 0; i < 2; ++i) {
        char link[64], dev[PATH_MAX];
        snprintf(link, sizeof(link), "/dev/disk/by-label/%s", LABELS[i]);
        if (!realpath(link, dev) || is_mounted(dev)) {
            continue;
        }
        if (strcmp(g.dev, dev) != 0) {
            snprintf(g.dev, sizeof(g.dev), "%s", dev);
            g.tries = 0;
            g.next = 0;
        }
        uint64_t now = plat_now_ms();
        if (now >= g.next && g.tries < MOUNT_TRIES) {
            const char *udisks[] = {"udisksctl", "mount", "-b", dev, "--no-user-interaction", NULL};
            const char *gio[] = {"gio", "mount", "-d", dev, NULL};
            LOG_I("bootsel: mounting %s (%s)", dev, LABELS[i]);
            posix_spawn_detached(g.tries % 2 ? gio : udisks);
            g.tries++;
            g.next = now + MOUNT_RETRY_MS;
        }
        out->stuck = out->stuck || (g.tries >= MOUNT_TRIES && now >= g.next);
    }
}

void plat_bootsel_scan(struct plat_bootsel *out)
{
    memset(out, 0, sizeof(*out));
    out->devices = plat_usb_count(0x2e8a, 0x0003) + plat_usb_count(0x2e8a, 0x000f); /* RP2040, RP2350 */
    out->drives = plat_uf2_drives(out->path, PLAT_BOOTSEL_MAX);
    if (out->devices > out->drives) {
        automount(out);
    }
}
