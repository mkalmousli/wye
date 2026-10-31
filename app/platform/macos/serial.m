/* macOS: USB serial ports (/dev/cu.*) with their USB vendor and product ids. */
#include "plat_serial.h"
#include "macos_iokit.h"

#include <IOKit/IOKitLib.h>
#include <string.h>

/* Walks up the IORegistry from a serial port to the USB device that owns it. */
static bool usb_owner(io_object_t start, struct plat_port_info *p)
{
    io_object_t cur = start;
    IOObjectRetain(cur);
    for (int depth = 0; depth < 12; depth++) {
        if (iokit_u16(cur, CFSTR("idVendor"), &p->vid) && iokit_u16(cur, CFSTR("idProduct"), &p->pid)) {
            iokit_str(cur, CFSTR("USB Product Name"), p->product, sizeof(p->product));
            iokit_str(cur, CFSTR("USB Serial Number"), p->serial, sizeof(p->serial));
            IOObjectRelease(cur);
            return true;
        }
        io_object_t parent = 0;
        kern_return_t kr = IORegistryEntryGetParentEntry(cur, kIOServicePlane, &parent);
        IOObjectRelease(cur);
        if (kr != KERN_SUCCESS || !parent) {
            return false;
        }
        cur = parent;
    }
    IOObjectRelease(cur);
    return false;
}

int plat_serial_list(struct plat_port_info *out, int max)
{
    io_iterator_t it = 0;
    if (IOServiceGetMatchingServices(0, IOServiceMatching("IOSerialBSDClient"), &it) != KERN_SUCCESS) {
        return 0;
    }
    int n = 0;
    io_object_t s;
    while (n < max && (s = IOIteratorNext(it))) {
        char path[PLAT_PORT_PATH_MAX];
        iokit_str(s, CFSTR("IOCalloutDevice"), path, sizeof(path));
        if (strncmp(path, "/dev/cu.", 8) == 0) {
            struct plat_port_info *p = &out[n];
            memset(p, 0, sizeof(*p));
            strcpy(p->path, path);
            if (usb_owner(s, p)) {
                n++;
            }
        }
        IOObjectRelease(s);
    }
    IOObjectRelease(it);
    return n;
}
