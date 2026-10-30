/* Unit + simulation tests: protocol, firmware logic (devcore), host core. */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>

bool step_tests(bool run_them)
{
    strlist objs = {0}, ld = {0};
    unit u = {.name = "tests"};
    core_flags(&u.cflags);
    sl_pushn(&u.cflags, "-Itests", "-Idevcore", (char *)NULL);
    fs_glob("tests", ".c", false, &u.srcs);
    fs_glob("devcore", ".c", false, &u.srcs);
    bool ok = step_core(&objs) && unit_compile(&u) && step_third_party(&objs, true);
    sl_extend(&objs, &u.objs);
    if (B.os == OS_WINDOWS) {
        sl_push(&ld, "-static");
    }
    core_libs(&ld);
    raylib_libs(&ld);
    char *exe = str_fmt("%s/wye_tests%s", B.out, B.exe_ext);
    ok = ok && link_exe(exe, &objs, &ld);
    if (ok && run_them && B.cross) {
        printf("build: built %s (cross build: run it on the target system)\n", exe);
    } else if (ok && run_them) {
        strlist cmd = {0};
        sl_push(&cmd, exe);
        printf("  TEST  %s\n", exe);
        ok = run(&cmd);
        sl_free(&cmd);
    }
    free(exe);
    sl_free(&objs);
    sl_free(&ld);
    return ok;
}
