/* USB serial ports: enumeration with USB identity, and non-blocking I/O. */
#ifndef WYE_PLAT_SERIAL_H
#define WYE_PLAT_SERIAL_H

#include "plat_thread.h"

#include <stddef.h>
#include <stdint.h>

#define PLAT_PORT_PATH_MAX 256
#define PLAT_PORT_STR_MAX 64

struct plat_port_info {
    char path[PLAT_PORT_PATH_MAX];     /* /dev/ttyACM0, COM5, /dev/cu.usbmodem1101 */
    uint16_t vid, pid;                 /* 0 if not a USB device */
    char product[PLAT_PORT_STR_MAX];
    char serial[PLAT_PORT_STR_MAX];
};

typedef struct plat_serial plat_serial;

int plat_serial_list(struct plat_port_info *out, int max);

/* Opens raw 8N1 with DTR asserted. Returns NULL and fills err on failure. */
plat_serial *plat_serial_open(const char *path, char *err, size_t errlen);
/*
 * Waits up to timeout_ms for data or for `wake` to be signalled.
 * Returns bytes read (0 on timeout/wake) or -1 if the device is gone.
 */
int plat_serial_read(plat_serial *s, uint8_t *buf, size_t cap, int timeout_ms, plat_event *wake);
/* Thread-safe with respect to a concurrent read. -1 if the device is gone. */
int plat_serial_write(plat_serial *s, const uint8_t *data, size_t len);
void plat_serial_close(plat_serial *s);

/* Opens and closes the port at 1200 baud: asks Pico-SDK firmware to enter BOOTSEL. */
bool plat_serial_touch_1200(const char *path);

#endif
