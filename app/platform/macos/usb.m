/* macOS: USB devices from the IORegistry (IOKit). */
#include "plat_sys.h"
#include "macos_iokit.h"

#include <IOKit/IOKitLib.h>

/* Calls fn for every USB device; fn returns true to stop early. */
typedef bool (*usb_visit)(io_object_t dev, void *arg);

static void each_usb(usb_visit fn, void *arg)
{
    io_iterator_t it = 0;
    if (IOServiceGetMatchingServices(0, IOServiceMatching("IOUSBHostDevice"), &it) != KERN_SUCCESS) {
        return;
    }
    io_object_t dev;
    while ((dev = IOIteratorNext(it))) {
        bool stop = fn(dev, arg);
        IOObjectRelease(dev);
        if (stop) {
            break;
        }
    }
    IOObjectRelease(it);
}

struct count_arg { uint16_t vid, pid; int n; };

static bool count_one(io_object_t dev, void *arg)
{
    struct count_arg *c = arg;
    uint16_t v = 0, p = 0;
    if (iokit_u16(dev, CFSTR("idVendor"), &v) && iokit_u16(dev, CFSTR("idProduct"), &p) &&
        (c->vid == 0 || v == c->vid) && (c->pid == 0 || p == c->pid)) {
        c->n++;
    }
    return false;
}

/* Devices with this vendor and product id (0 matches any). */
int plat_usb_count(uint16_t vid, uint16_t pid)
{
    struct count_arg c = {vid, pid, 0};
    each_usb(count_one, &c);
    return c.n;
}

struct list_arg { struct plat_usb_device *out; int max, n; };

static bool list_one(io_object_t dev, void *arg)
{
    struct list_arg *l = arg;
    if (l->n >= l->max) {
        return true;
    }
    struct plat_usb_device *d = &l->out[l->n];
    if (!iokit_u16(dev, CFSTR("idVendor"), &d->vid) || !iokit_u16(dev, CFSTR("idProduct"), &d->pid)) {
        return false;
    }
    iokit_str(dev, CFSTR("USB Vendor Name"), d->manufacturer, sizeof(d->manufacturer));
    iokit_str(dev, CFSTR("USB Product Name"), d->product, sizeof(d->product));
    iokit_str(dev, CFSTR("USB Serial Number"), d->serial, sizeof(d->serial));
    l->n++;
    return false;
}

int plat_usb_list(struct plat_usb_device *out, int max)
{
    struct list_arg l = {out, max, 0};
    each_usb(list_one, &l);
    return l.n;
}
