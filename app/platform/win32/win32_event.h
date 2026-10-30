/* Win32 event internals: serial reads wait on the event handle directly. */
#ifndef WYE_WIN32_EVENT_H
#define WYE_WIN32_EVENT_H

#include "plat_thread.h"

#include <windows.h>

HANDLE plat_event_handle(plat_event *e);

#endif
