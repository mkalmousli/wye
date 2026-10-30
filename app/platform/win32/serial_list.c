/* Enumerates COM ports with USB VID/PID and the device's USB product string. */
#include "plat_serial.h"

#include <windows.h>
#include <initguid.h> /* must precede devpkey.h so the keys get defined */
#include <devpkey.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_GUID(WYE_GUID_COMPORT, 0x86e0d1e0, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);

static void parse_ids(const char *hwid, struct plat_port_info *p)
{
    const char *v = strstr(hwid, "VID_");
    const char *d = strstr(hwid, "PID_");
    if (v && d) {
        p->vid = (uint16_t)strtoul(v + 4, NULL, 16);
        p->pid = (uint16_t)strtoul(d + 4, NULL, 16);
    }
}

static void port_name(HDEVINFO set, SP_DEVINFO_DATA *dev, char *out, size_t len)
{
    HKEY key = SetupDiOpenDevRegKey(set, dev, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    DWORD size = (DWORD)len, type = 0;
    out[0] = '\0';
    if (key != INVALID_HANDLE_VALUE) {
        RegQueryValueExA(key, "PortName", NULL, &type, (BYTE *)out, &size);
        RegCloseKey(key);
    }
}

/* The USB product string ("Wye Bridge") as reported by the device. */
static void bus_description(HDEVINFO set, SP_DEVINFO_DATA *dev, char *out, size_t len)
{
    WCHAR wide[PLAT_PORT_STR_MAX];
    DEVPROPTYPE type;
    out[0] = '\0';
    if (SetupDiGetDevicePropertyW(set, dev, &DEVPKEY_Device_BusReportedDeviceDesc, &type, (BYTE *)wide,
                                  sizeof(wide), NULL, 0)) {
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)len, NULL, NULL);
    }
}

int plat_serial_list(struct plat_port_info *out, int max)
{
    HDEVINFO set = SetupDiGetClassDevsA(&WYE_GUID_COMPORT, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE) {
        return 0;
    }
    int n = 0;
    SP_DEVINFO_DATA dev = {.cbSize = sizeof(dev)};
    for (DWORD i = 0; n < max && SetupDiEnumDeviceInfo(set, i, &dev); ++i) {
        struct plat_port_info *p = &out[n];
        char hwid[512] = "", name[32];
        memset(p, 0, sizeof(*p));
        SetupDiGetDeviceRegistryPropertyA(set, &dev, SPDRP_HARDWAREID, NULL, (BYTE *)hwid, sizeof(hwid), NULL);
        port_name(set, &dev, name, sizeof(name));
        if (strncmp(name, "COM", 3) != 0) {
            continue;
        }
        snprintf(p->path, sizeof(p->path), "\\\\.\\%s", name); /* needed for COM10+ */
        parse_ids(hwid, p);
        bus_description(set, &dev, p->product, sizeof(p->product));
        n++;
    }
    SetupDiDestroyDeviceInfoList(set);
    return n;
}
