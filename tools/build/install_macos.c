/* macOS install: a Wye.app bundle in ~/Applications (or --prefix). */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>

static const char *const PLIST =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
    "<plist version=\"1.0\"><dict>\n"
    "  <key>CFBundleName</key><string>Wye</string>\n"
    "  <key>CFBundleDisplayName</key><string>Wye</string>\n"
    "  <key>CFBundleIdentifier</key><string>io.github.mkalmousli.wye</string>\n"
    "  <key>CFBundleExecutable</key><string>Wye</string>\n"
    "  <key>CFBundleIconFile</key><string>wye</string>\n"
    "  <key>CFBundlePackageType</key><string>APPL</string>\n"
    "  <key>CFBundleShortVersionString</key><string>%s</string>\n"
    "  <key>CFBundleVersion</key><string>%s</string>\n"
    "  <key>LSMinimumSystemVersion</key><string>11.0</string>\n"
    "  <key>LSApplicationCategoryType</key><string>public.app-category.utilities</string>\n"
    "  <key>NSHighResolutionCapable</key><true/>\n"
    "</dict></plist>\n";

/* wye.icns from the 512 px PNG with the system's sips and iconutil. */
static void icon(const char *bundle)
{
    static const int sizes[] = {16, 32, 128, 256, 512};
    char *set = str_fmt("%s/wye.iconset", B.out);
    fs_mkdirs(set);
    for (int i = 0; i < 5; ++i) {
        char *cmd = str_fmt("sips -z %d %d app/resources/icons/wye.png --out '%s/icon_%dx%d.png'", sizes[i],
                            sizes[i], set, sizes[i], sizes[i]);
        inst_optional(cmd);
        free(cmd);
    }
    char *cmd = str_fmt("iconutil -c icns '%s' -o '%s/Contents/Resources/wye.icns'", set, bundle);
    inst_optional(cmd);
    free(cmd);
    free(set);
}

bool install_macos(bool install)
{
    char *dir = B.prefix ? str_fmt("%s", B.prefix) : inst_home("Applications");
    char *bundle = str_fmt("%s/Wye.app", dir);
    bool ok = true;
    if (install) {
        char *exe = str_fmt("%s/Wye", B.out), *bin = str_fmt("%s/Contents/MacOS/Wye", bundle);
        char *plist = str_fmt(PLIST, B.version, B.version), *plist_path = str_fmt("%s/Contents/Info.plist", bundle);
        char *res = str_fmt("%s/Contents/Resources", bundle);
        ok = inst_copy(exe, bin, true) && inst_write(plist_path, plist) && fs_mkdirs(res);
        if (ok) {
            icon(bundle);
            printf("build: Wye is installed in %s\n", dir);
        }
        free(exe), free(bin), free(plist), free(plist_path), free(res);
    } else if (fs_exists(bundle)) {
        strlist cmd = {0};
        sl_pushn(&cmd, "rm", "-rf", bundle, (char *)NULL);
        ok = run(&cmd);
        printf("  RM    %s\n", bundle);
        sl_free(&cmd);
    }
    free(dir);
    free(bundle);
    return ok;
}
