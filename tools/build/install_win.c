/* Windows install: %LOCALAPPDATA%\Programs\Wye plus a Start menu shortcut. */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>

static char *env_path(const char *var, const char *sub)
{
    const char *base = getenv(var);
    if (!base || !*base) {
        fprintf(stderr, "build: %%%s%% is not set\n", var);
        exit(1);
    }
    return str_fmt("%s\\%s", base, sub);
}

static bool shortcut(const char *lnk, const char *exe)
{
    strlist cmd = {0};
    sl_pushn(&cmd, "powershell", "-NoProfile", "-Command", (char *)NULL);
    sl_pushf(&cmd, "$s=(New-Object -ComObject WScript.Shell).CreateShortcut('%s');$s.TargetPath='%s';"
                   "$s.Description='Wye';$s.Save()", lnk, exe);
    bool ok = run(&cmd);
    printf("  %s  %s\n", ok ? "INST" : "FAIL", lnk);
    sl_free(&cmd);
    return ok;
}

bool install_win(bool install)
{
    char *dir = B.prefix ? str_fmt("%s", B.prefix) : env_path("LOCALAPPDATA", "Programs\\Wye");
    char *exe = str_fmt("%s\\Wye.exe", dir);
    char *lnk = env_path("APPDATA", "Microsoft\\Windows\\Start Menu\\Programs\\Wye.lnk");
    bool ok = true;
    if (install) {
        char *built = str_fmt("%s/Wye.exe", B.out);
        fs_mkdirs(dir);
        ok = inst_copy(built, exe, true) && shortcut(lnk, exe);
        if (ok) {
            printf("build: Wye is installed - find it in the Start menu\n");
        }
        free(built);
    } else {
        inst_remove(exe);
        inst_remove(lnk);
        char *rmdir = str_fmt("cmd /c rmdir \"%s\"", dir);
        inst_optional(rmdir); /* only when empty */
        free(rmdir);
    }
    free(dir), free(exe), free(lnk);
    return ok;
}
