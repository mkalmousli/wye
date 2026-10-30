/* The single Wye binary: core + UI + embedded firmware and fonts. */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>

static bool embeds(strlist *objs)
{
    static const char *const files[][2] = {
        {"wye_fw_bridge_uf2", "wye_bridge.uf2"},
        {"wye_fw_target_uf2", "wye_target.uf2"},
        {"wye_font_regular", "app/resources/fonts/Inter-Regular.ttf"},
        {"wye_font_bold", "app/resources/fonts/Inter-SemiBold.ttf"},
        {"wye_font_heavy", "app/resources/fonts/Inter-Bold.ttf"},
    };
    unit u = {.name = "embed"};
    base_flags(&u.cflags, true);
    for (int i = 0; i < (int)(sizeof(files) / sizeof(files[0])); ++i) {
        char *input = i < 2 ? str_fmt("%s/%s", B.fw_dir, files[i][1]) : str_fmt("%s", files[i][1]);
        char *gen = NULL;
        bool ok = embed_file(files[i][0], input, &gen);
        if (ok) {
            sl_push(&u.srcs, gen);
        }
        free(input);
        free(gen);
        if (!ok) {
            return false;
        }
    }
    bool ok = unit_compile(&u);
    sl_extend(objs, &u.objs);
    return ok;
}

static bool ui(strlist *objs)
{
    unit u = {.name = "ui"};
    core_flags(&u.cflags);
    sl_pushn(&u.cflags, "-Iapp", "-Iapp/ui", (char *)NULL);
    if (B.os == OS_LINUX) {
        sl_push(&u.cflags, "-Iapp/platform/linux");
    }
    sl_pushn(&u.srcs, "app/main.c", "app/app_modes.c", (char *)NULL);
    fs_glob("app/ui", ".c", false, &u.srcs);
    bool ok = unit_compile(&u);
    sl_extend(objs, &u.objs);
    return ok;
}

/* Windows icon, manifest and version info. */
static bool resources(strlist *objs)
{
    const char *rc = "app/resources/windows/wye.rc";
    char *obj = str_fmt("%s/obj/wye.res.o", B.out);
    strlist inputs = {0}, cmd = {0};
    sl_pushn(&inputs, rc, "app/resources/windows/wye.ico", "app/resources/windows/wye.manifest", "VERSION",
             (char *)NULL);
    bool ok = true;
    if (newer_than(obj, &inputs)) {
        fs_mkdir_parent(obj);
        sl_pushn(&cmd, B.windres, "-Iapp/resources/windows", (char *)NULL);
        sl_pushf(&cmd, "-DWYE_RC_VERSION=%d,%d,%d,0", B.major, B.minor, B.patch);
        sl_pushn(&cmd, rc, "-O", "coff", "-o", obj, (char *)NULL);
        printf("  RC    %s\n", rc);
        ok = run(&cmd);
    }
    sl_push(objs, obj);
    free(obj);
    sl_free(&inputs);
    sl_free(&cmd);
    return ok;
}

bool step_app(void)
{
    strlist objs = {0}, ld = {0};
    bool ok = step_core(&objs) && ui(&objs) && embeds(&objs) && step_third_party(&objs, true) &&
              (B.os != OS_WINDOWS || resources(&objs));
    if (!ok) {
        return false;
    }
    if (B.os == OS_WINDOWS) {
        sl_pushn(&ld, "-static", "-municode", "-mwindows", (char *)NULL);
    }
    core_libs(&ld);
    raylib_libs(&ld);
    char *exe = str_fmt("%s/%s%s", B.out, B.os == OS_LINUX ? "wye" : "Wye", B.exe_ext);
    ok = link_exe(exe, &objs, &ld);
    if (ok) {
        printf("build: built %s\n", exe);
    }
    free(exe);
    sl_free(&objs);
    sl_free(&ld);
    return ok;
}
