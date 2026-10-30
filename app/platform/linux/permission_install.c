/*
 * Privileged helper (runs as root via pkexec): installs udev rules that give
 * the logged-in seat user access (systemd "uaccess") to input devices,
 * /dev/uinput and Wye/Pico serial ports, and loads uinput at boot.
 */
#define _DEFAULT_SOURCE
#include "plat_linux.h"
#include "proc.h"

#include <stdio.h>
#include <unistd.h>

static const char RULES[] =
    "# Installed by Wye. Remove this file to revoke access.\n"
    "KERNEL==\"uinput\", SUBSYSTEM==\"misc\", TAG+=\"uaccess\", OPTIONS+=\"static_node=uinput\"\n"
    "SUBSYSTEM==\"input\", KERNEL==\"event*\", TAG+=\"uaccess\"\n"
    "SUBSYSTEM==\"tty\", ATTRS{idVendor}==\"1209\", ATTRS{idProduct}==\"0001\", TAG+=\"uaccess\"\n"
    "SUBSYSTEM==\"tty\", ATTRS{idVendor}==\"2e8a\", ATTRS{idProduct}==\"000a\", TAG+=\"uaccess\"\n"
    "SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"2e8a\", ATTRS{idProduct}==\"0003\", TAG+=\"uaccess\"\n";

static bool write_text(const char *path, const char *text)
{
    FILE *f = fopen(path, "w");
    if (!f) {
        return false;
    }
    fputs(text, f);
    return fclose(f) == 0;
}

int wye_linux_install_permissions(void)
{
    if (geteuid() != 0) {
        fprintf(stderr, "must run as root (use pkexec)\n");
        return 1;
    }
    if (!write_text("/etc/udev/rules.d/70-wye.rules", RULES) ||
        !write_text("/etc/modules-load.d/wye.conf", "uinput\n")) {
        fprintf(stderr, "cannot write udev rules\n");
        return 1;
    }
    const char *modprobe[] = {"modprobe", "uinput", NULL};
    const char *reload[] = {"udevadm", "control", "--reload-rules", NULL};
    const char *trigger[] = {"udevadm", "trigger", "--action=change", NULL};
    posix_run(modprobe);
    posix_run(reload);
    posix_run(trigger);
    return 0;
}
