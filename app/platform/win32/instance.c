/* Single instance via a named mutex; a second launch signals "show window". */
#include "plat_sys.h"

#include <windows.h>

#define MUTEX_NAME "Local\\Wye.Instance"
#define SHOW_EVENT "Local\\Wye.Show"

static HANDLE g_mutex, g_show;

bool plat_instance_acquire(bool notify)
{
    HANDLE m = CreateMutexA(NULL, TRUE, MUTEX_NAME);
    if (m && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(m);
        HANDLE ev = notify ? OpenEventA(EVENT_MODIFY_STATE, FALSE, SHOW_EVENT) : NULL;
        if (ev) {
            SetEvent(ev);
            CloseHandle(ev);
        }
        return false;
    }
    g_mutex = m; /* held for the life of the process */
    g_show = CreateEventA(NULL, FALSE, FALSE, SHOW_EVENT);
    return true;
}

bool plat_instance_show_requested(void)
{
    return g_show && WaitForSingleObject(g_show, 0) == WAIT_OBJECT_0;
}
