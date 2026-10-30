/*
 * Pico firmware (bridge + target). The pico-sdk only supports CMake, so this
 * step drives it (with Ninja when available) and collects the .uf2 files in
 * out/firmware/, where the app build embeds them from.
 */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FW_BUILD "out/firmware/build"

/* A cache made with another pico-sdk cannot be reconfigured: start over. */
static void forget_other_sdk(const char *sdk)
{
    char *cache = fs_read(FW_BUILD "/CMakeCache.txt", NULL);
    char *want = str_fmt("PICO_SDK_PATH:PATH=%s\n", sdk);
    if (cache && !strstr(cache, want)) {
        strlist rm = {0};
        sl_pushn(&rm, "cmake", "-E", "rm", "-rf", FW_BUILD, (char *)NULL);
        printf("  CLEAN firmware (pico-sdk changed)\n");
        run(&rm);
        sl_free(&rm);
    }
    free(want);
    free(cache);
}

static bool configure(const char *sdk)
{
    strlist cmd = {0};
    char *hash = run_capture("git rev-parse --short=8 HEAD");
    sl_pushn(&cmd, "cmake", "-S", "firmware", "-B", FW_BUILD, (char *)NULL);
    if (!fs_exists(FW_BUILD "/CMakeCache.txt")) {
        char *ninja = run_capture("ninja --version");
        if (ninja) {
            sl_pushn(&cmd, "-G", "Ninja", (char *)NULL);
        }
        free(ninja);
    }
    sl_pushf(&cmd, "-DPICO_SDK_PATH=%s", sdk);
    sl_pushf(&cmd, "-DWYE_VERSION=%s", B.version);
    sl_pushf(&cmd, "-DWYE_BUILD_ID=%s", hash ? hash : "0");
    printf("  CMAKE firmware (pico-sdk: %s)\n", sdk);
    bool ok = run(&cmd);
    free(hash);
    sl_free(&cmd);
    return ok;
}

static bool collect(const char *role)
{
    char *from = str_fmt(FW_BUILD "/%s/wye_%s.uf2", role, role);
    char *to = str_fmt("out/firmware/wye_%s.uf2", role);
    bool ok = fs_copy(from, to);
    printf(ok ? "build: built %s\n" : "build: missing %s\n", ok ? to : from);
    free(from);
    free(to);
    return ok;
}

bool step_firmware(void)
{
    const char *sdk = B.pico_sdk ? B.pico_sdk : getenv("PICO_SDK_PATH");
    if (!sdk || !*sdk) {
        fprintf(stderr, "build: firmware needs the Raspberry Pi pico-sdk and arm-none-eabi-gcc.\n"
                        "     Clone https://github.com/raspberrypi/pico-sdk (with submodules), then run\n"
                        "     ./build.c firmware --pico-sdk=/path/to/pico-sdk   (or set PICO_SDK_PATH)\n");
        return false;
    }
    forget_other_sdk(sdk);
    fs_mkdirs(FW_BUILD);
    strlist cmd = {0};
    sl_pushn(&cmd, "cmake", "--build", FW_BUILD, "--parallel", (char *)NULL);
    sl_pushf(&cmd, "%d", B.jobs);
    bool ok = configure(sdk) && run(&cmd) && collect("bridge") && collect("target");
    sl_free(&cmd);
    return ok;
}
