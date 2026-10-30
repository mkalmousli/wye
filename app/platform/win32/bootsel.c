/* Picos in BOOTSEL mode: counted on USB via SetupAPI; Windows mounts their drive itself. */
#include "plat_sys.h"
#include "plat_thread.h"
#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <string.h>

#define STUCK_AFTER_MS 8000u

int plat_usb_count(uint16_t vid, uint16_t pid)
{
    char want[32];
    snprintf(want, sizeof(want), "VID_%04X&PID_%04X", vid, pid);
    HDEVINFO set = SetupDiGetClassDevsA(NULL, "USB", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (set == INVALID_HANDLE_VALUE) {
        return 0;
    }
    int n = 0;
    SP_DEVINFO_DATA dev = {.cbSize = sizeof(dev)};
    for (DWORD i = 0; SetupDiEnumDeviceInfo(set, i, &dev); ++i) {
        char id[256] = "";
        SetupDiGetDeviceInstanceIdA(set, &dev, id, sizeof(id), NULL);
        CharUpperA(id);
        n += strstr(id, want) && !strstr(id, "&MI_"); /* the device, not its interfaces */
    }
    SetupDiDestroyDeviceInfoList(set);
    return n;
}

void plat_bootsel_scan(struct plat_bootsel *out)
{
    static uint64_t waiting_since;
    memset(out, 0, sizeof(*out));
    out->devices = plat_usb_count(0x2e8a, 0x0003) + plat_usb_count(0x2e8a, 0x000f); /* RP2040, RP2350 */
    out->drives = plat_uf2_drives(out->path, PLAT_BOOTSEL_MAX);
    if (out->devices > out->drives) {
        uint64_t now = plat_now_ms();
        waiting_since = waiting_since ? waiting_since : now;
        out->stuck = now - waiting_since > STUCK_AFTER_MS;
    } else {
        waiting_since = 0;
    }
}
