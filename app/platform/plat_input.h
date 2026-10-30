/*
 * Global keyboard/mouse capture.
 *   Windows: low-level hooks on a dedicated thread.
 *   macOS:   CGEventTap on a dedicated run loop (Accessibility permission).
 *   Linux:   evdev with grab + uinput re-injection (works on X11 and Wayland).
 */
#ifndef WYE_PLAT_INPUT_H
#define WYE_PLAT_INPUT_H

#include "input_event.h"

#include <stddef.h>

enum plat_input_status {
    PLAT_INPUT_STOPPED = 0,
    PLAT_INPUT_RUNNING,
    PLAT_INPUT_NO_PERMISSION,
    PLAT_INPUT_FAILED,
};

bool plat_input_start(wye_input_sink sink, void *ctx, char *err, size_t errlen);
void plat_input_stop(void);
enum plat_input_status plat_input_status(void);

/*
 * Forwarding mode: the pointer is frozen locally (deltas still reported) and
 * every event goes to the sink for swallowing. Called from any thread.
 */
void plat_input_set_forwarding(bool forwarding);

/* While forwarding, leave the pointer working on this computer (mouse not shared). */
void plat_input_set_pointer_local(bool local);

/* Mirror the target's lock-key LEDs on local keyboards where supported. */
void plat_input_set_leds(uint8_t leds);

#endif
