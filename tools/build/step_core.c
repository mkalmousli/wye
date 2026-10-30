/* wyecore: everything except the UI and main(), shared by the app and the tests. */
#include "bld.h"

void core_flags(strlist *f)
{
    base_flags(f, true);
    raylib_public_flags(f);
    sl_pushn(f, "-std=c11", "-Iapp/core", "-Iapp/core/input", "-Iapp/platform", "-Iproto",
             "-Ithird_party/monocypher", (char *)NULL);
    miniaudio_flags(f);
    sl_push(f, B.os == OS_WINDOWS ? "-Iapp/platform/win32" : "-Iapp/platform/posix");
    sl_pushf(f, "-DWYE_VERSION_MAJOR=%d", B.major);
    sl_pushf(f, "-DWYE_VERSION_MINOR=%d", B.minor);
    sl_pushf(f, "-DWYE_VERSION_PATCH=%d", B.patch);
    sl_pushf(f, "-DWYE_VERSION_STRING=\"%s\"", B.version);
    static char *rel; /* read once: a missing key warns only once */
    if (!rel) {
        rel = pubkey_hex("release");
    }
    sl_pushf(f, "-DWYE_RELEASE_PUBKEY_HEX=\"%s\"", rel);
}

static void platform_sources(strlist *s)
{
    if (B.os == OS_WINDOWS) {
        fs_glob("app/platform/win32", ".c", false, s);
    } else {
        fs_glob("app/platform/posix", ".c", false, s);
        fs_glob(B.os == OS_MACOS ? "app/platform/macos" : "app/platform/linux", ".c", false, s);
    }
}

/* OS libraries wyecore needs (raylib's come from raylib_libs). */
void core_libs(strlist *f)
{
    if (B.os == OS_WINDOWS) {
        sl_pushn(f, "-lsetupapi", "-lwinhttp", "-lbcrypt", "-lshlwapi", "-lole32", "-luuid", (char *)NULL);
    } else if (B.os == OS_MACOS) {
        sl_pushn(f, "-framework", "ApplicationServices", "-framework", "IOKit", "-framework", "CoreFoundation",
                 "-framework", "AppKit", "-lobjc", "-framework", "CoreAudio", "-framework", "AudioToolbox",
                 (char *)NULL);
    } else {
        sl_push(f, "-ldl"); /* miniaudio loads PulseAudio/ALSA at runtime */
    }
    sl_pushn(f, "-lm", "-lpthread", (char *)NULL);
}

bool step_core(strlist *objs)
{
    unit u = {.name = "core"};
    core_flags(&u.cflags);
    fs_glob("app/core", ".c", true, &u.srcs);
    fs_glob("proto", ".c", false, &u.srcs);
    platform_sources(&u.srcs);
    bool ok = unit_compile(&u);
    sl_extend(objs, &u.objs);
    return ok;
}
