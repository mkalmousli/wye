/*
 * Self-update on Windows: a running .exe can be renamed but not overwritten,
 * so it is moved aside to .old, the new one takes its name, and the new
 * process is started (it removes the .old file once the old one has exited).
 */
#include "plat_sys.h"

#include <windows.h>
#include <stdio.h>

static void start(const char *exe)
{
    char cmd[PLAT_PATH_MAX + 32];
    STARTUPINFOA si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi;
    snprintf(cmd, sizeof(cmd), "\"%s\" --updated", exe);
    if (CreateProcessA(exe, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

bool plat_install_update(const char *downloaded, char *err, size_t errlen)
{
    char exe[PLAT_PATH_MAX], old[PLAT_PATH_MAX + 8];
    if (!plat_exe_path(exe, sizeof(exe))) {
        snprintf(err, errlen, "cannot locate the running program");
        return false;
    }
    snprintf(old, sizeof(old), "%s.old", exe);
    DeleteFileA(old);
    if (!MoveFileExA(exe, old, MOVEFILE_REPLACE_EXISTING)) {
        snprintf(err, errlen, "no permission to replace %s (error %lu)", exe, GetLastError());
        return false;
    }
    if (!MoveFileExA(downloaded, exe, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
        MoveFileExA(old, exe, MOVEFILE_REPLACE_EXISTING); /* roll back */
        snprintf(err, errlen, "installing the update failed (error %lu)", GetLastError());
        return false;
    }
    start(exe);
    return true;
}

/* Called at startup: remove the previous version left behind by an update. */
void plat_cleanup_previous_version(void)
{
    char exe[PLAT_PATH_MAX], old[PLAT_PATH_MAX + 8];
    if (plat_exe_path(exe, sizeof(exe))) {
        snprintf(old, sizeof(old), "%s.old", exe);
        for (int i = 0; i < 20 && plat_path_exists(old) && !DeleteFileA(old); ++i) {
            Sleep(250); /* the old process may still be exiting */
        }
    }
}
