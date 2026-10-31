/* macOS: IORegistry property helpers shared by the USB and serial code. */
#ifndef WYE_MACOS_IOKIT_H
#define WYE_MACOS_IOKIT_H

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Reads a number property into out; false if the property is missing. */
bool iokit_u16(io_object_t dev, CFStringRef key, uint16_t *out);
/* Reads a string property into out (empty if missing). */
void iokit_str(io_object_t dev, CFStringRef key, char *out, size_t len);

#endif
