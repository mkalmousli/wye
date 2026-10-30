/* Target firmware internals shared between main.c and the USB class callbacks. */
#ifndef WYE_TARGET_MAIN_H
#define WYE_TARGET_MAIN_H

#include "target.h"

struct wye_target *target_get(void);
/* One pass of link and report work, without tud_task(): safe inside USB callbacks. */
void target_pump(void);

#endif
