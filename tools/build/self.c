/* The tool maintaining itself: self-rebuild and clean. */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Recompiles this tool when build.c or tools/build/ changed, then re-runs it. */
void rebuild_self(char **argv)
{
    const char *ext = B.exe_ext && strstr(argv[0], ".exe") == NULL && B.os == OS_WINDOWS && !B.cross ? ".exe" : "";
    char *exe = str_fmt("%s%s", argv[0], ext);
    strlist inputs = {0}, cmd = {0};
    sl_push(&inputs, "build.c");
    fs_glob("tools/build", ".c", false, &inputs);
    fs_glob("tools/build", ".h", false, &inputs);
    if (fs_exists(exe) && newer_than(exe, &inputs)) {
        const char *cc = getenv("BLD_CC") ? getenv("BLD_CC") : B.os == OS_WINDOWS && !B.cross ? "gcc" : "cc";
        char *fresh = str_fmt("%s.new", exe), *old = str_fmt("%s.old", exe);
        printf("build: build tool changed, rebuilding it\n");
        sl_pushn(&cmd, cc, "-O1", "-o", fresh, "build.c", (char *)NULL);
        if (!run(&cmd)) {
            exit(1);
        }
        remove(old);
        fs_rename(exe, old); /* Windows cannot overwrite a running .exe */
        if (!fs_rename(fresh, exe)) {
            fprintf(stderr, "build: cannot replace %s\n", exe);
            exit(1);
        }
        remove(old);
        exec_self(exe, argv);
        fprintf(stderr, "build: cannot restart %s\n", exe);
        exit(1);
    }
    free(exe);
    sl_free(&inputs);
}

static void remove_entry(void *ctx, const char *name, bool is_dir)
{
    strlist cmd = {0};
    (void)is_dir;
    if (B.os == OS_WINDOWS && !B.cross) {
        sl_pushn(&cmd, "cmd", "/c", (char *)NULL);
        sl_pushf(&cmd, "rmdir /s /q out\\%s 2>nul || del /q out\\%s", name, name);
    } else {
        sl_pushn(&cmd, "rm", "-rf", (char *)NULL);
        sl_pushf(&cmd, "out/%s", name);
    }
    *(bool *)ctx &= run(&cmd);
    sl_free(&cmd);
}

/* Deletes out/ except the hidden out/.tool, which holds this running tool. */
bool clean(void)
{
    bool ok = true;
    printf("  CLEAN out/\n");
    os_listdir("out", remove_entry, &ok);
    return ok;
}
