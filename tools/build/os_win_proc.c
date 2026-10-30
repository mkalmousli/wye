/* Windows primitives: waiting, capturing output, relaunching. */
#include "bld.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int proc_wait_any(const intptr_t *procs, int n, bool *ok)
{
    DWORD r = WaitForMultipleObjects((DWORD)n, (const HANDLE *)procs, FALSE, INFINITE);
    DWORD k = r - WAIT_OBJECT_0;
    int i = k < (DWORD)n ? (int)k : 0;
    DWORD code = 1;
    *ok = GetExitCodeProcess((HANDLE)procs[i], &code) && code == 0;
    CloseHandle((HANDLE)procs[i]);
    return i;
}

char *run_capture(const char *cmd)
{
    char *full = str_fmt("%s 2>NUL", cmd);
    FILE *p = _popen(full, "r");
    free(full);
    char buf[512] = "";
    size_t n = p ? fread(buf, 1, sizeof(buf) - 1, p) : 0;
    buf[n] = '\0';
    bool ok = p && _pclose(p) == 0;
    return ok ? str_fmt("%s", str_trim(buf)) : NULL;
}

/* A running .exe cannot be replaced by exec: run the new one and pass on its exit code. */
bool exec_self(const char *exe, char **argv)
{
    strlist args = {0};
    sl_push(&args, exe);
    for (int i = 1; argv[i]; ++i) {
        sl_push(&args, argv[i]);
    }
    bool ok = run(&args);
    exit(ok ? 0 : 1);
}

int cpu_count(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors > 0 ? (int)si.dwNumberOfProcessors : 1;
}
