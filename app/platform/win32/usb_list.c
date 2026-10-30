/* USB devices attached to this computer, via SetupAPI (interfaces and hubs left out). */
#include "plat_sys.h"
#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void prop(HDEVINFO set, SP_DEVINFO_DATA *dev, DWORD which, char *out, size_t len)
{
    out[0] = '\0';
    SetupDiGetDeviceRegistryPropertyA(set, dev, which, NULL, (BYTE *)out, (DWORD)len, NULL);
}

/* "USB\VID_046D&PID_C52B\5&2D0A&0&3": IDs, and a serial if the last part is a real one. */
static bool parse(const char *id, struct plat_usb_device *u)
{
    const char *v = strstr(id, "VID_"), *p = strstr(id, "PID_"), *last = strrchr(id, '\\');
    if (!v || !p || strstr(id, "&MI_") || strstr(id, "ROOT_HUB")) {
        return false;
    }
    u->vid = (uint16_t)strtoul(v + 4, NULL, 16);
    u->pid = (uint16_t)strtoul(p + 4, NULL, 16);
    if (last && !strchr(last, '&')) {
        snprintf(u->serial, sizeof(u->serial), "%s", last + 1);
    }
    return true;
}

int plat_usb_list(struct plat_usb_device *out, int max)
{
    HDEVINFO set = SetupDiGetClassDevsA(NULL, "USB", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (set == INVALID_HANDLE_VALUE) {
        return 0;
    }
    int n = 0;
    SP_DEVINFO_DATA dev = {.cbSize = sizeof(dev)};
    for (DWORD i = 0; n < max && SetupDiEnumDeviceInfo(set, i, &dev); ++i) {
        char id[256] = "";
        struct plat_usb_device *u = &out[n];
        memset(u, 0, sizeof(*u));
        SetupDiGetDeviceInstanceIdA(set, &dev, id, sizeof(id), NULL);
        if (parse(id, u)) {
            prop(set, &dev, SPDRP_MFG, u->manufacturer, sizeof(u->manufacturer));
            prop(set, &dev, SPDRP_DEVICEDESC, u->product, sizeof(u->product));
            n++;
        }
    }
    SetupDiDestroyDeviceInfoList(set);
    return n;
}
