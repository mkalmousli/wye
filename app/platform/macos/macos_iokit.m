/* macOS: IORegistry property helpers shared by the USB and serial code. */
#include "macos_iokit.h"

static CFTypeRef prop(io_object_t s, CFStringRef key)
{
    return IORegistryEntryCreateCFProperty(s, key, kCFAllocatorDefault, 0);
}

bool iokit_u16(io_object_t s, CFStringRef key, uint16_t *out)
{
    CFNumberRef n = (CFNumberRef)prop(s, key);
    if (!n) {
        return false;
    }
    int v = 0;
    CFNumberGetValue(n, kCFNumberIntType, &v);
    CFRelease(n);
    *out = (uint16_t)v;
    return true;
}

void iokit_str(io_object_t s, CFStringRef key, char *out, size_t len)
{
    out[0] = 0;
    CFStringRef str = (CFStringRef)prop(s, key);
    if (!str) {
        return;
    }
    if (CFGetTypeID(str) == CFStringGetTypeID()) {
        CFStringGetCString(str, out, (CFIndex)len, kCFStringEncodingUTF8);
    }
    CFRelease(str);
}
