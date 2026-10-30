/*
 * Linux desktop install: binary, launcher and icons under ~/.local (or
 * --prefix). The app asks for device permissions itself on first launch.
 */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>

static const char *const DESKTOP =
    "[Desktop Entry]\n"
    "Type=Application\n"
    "Name=Wye\n"
    "GenericName=Keyboard and Mouse Switch\n"
    "Comment=Drive a second computer with this keyboard and mouse, over a cable\n"
    "Exec=\"%s\"\n"
    "Icon=wye\n"
    "Terminal=false\n"
    "Categories=Utility;System;\n"
    "Keywords=kvm;keyboard;mouse;switch;remote;pico;\n"
    "StartupWMClass=Wye\n"
    "StartupNotify=true\n";

typedef struct {
    char *bin, *desktop, *svg, *png, *autostart, *share;
} linux_paths;

static linux_paths paths(void)
{
    char *root = B.prefix ? str_fmt("%s", B.prefix) : inst_home(".local");
    linux_paths p = {
        .bin = str_fmt("%s/bin/wye", root),
        .desktop = str_fmt("%s/share/applications/wye.desktop", root),
        .svg = str_fmt("%s/share/icons/hicolor/scalable/apps/wye.svg", root),
        .png = str_fmt("%s/share/icons/hicolor/256x256/apps/wye.png", root),
        .autostart = B.prefix ? NULL : inst_home(".config/autostart/wye.desktop"),
        .share = str_fmt("%s/share", root),
    };
    free(root);
    return p;
}

static void refresh_caches(const linux_paths *p)
{
    char *cmd = str_fmt("update-desktop-database '%s/applications'", p->share);
    inst_optional(cmd);
    free(cmd);
    cmd = str_fmt("gtk-update-icon-cache -q -t -f '%s/icons/hicolor'", p->share);
    inst_optional(cmd);
    free(cmd);
}

bool install_linux(bool install)
{
    linux_paths p = paths();
    bool ok = true;
    if (install) {
        char *exe = str_fmt("%s/wye", B.out), *desktop = str_fmt(DESKTOP, p.bin);
        ok = inst_copy(exe, p.bin, true) && inst_copy("app/resources/icons/wye.svg", p.svg, false) &&
             inst_copy("app/resources/linux/wye.png", p.png, false) && inst_write(p.desktop, desktop);
        free(exe);
        free(desktop);
    } else {
        const char *files[] = {p.bin, p.desktop, p.svg, p.png, p.autostart};
        for (int i = 0; i < 5; ++i) {
            if (files[i]) {
                inst_remove(files[i]);
            }
        }
    }
    refresh_caches(&p);
    if (ok && install) {
        printf("build: Wye is installed - open it from your app menu%s\n",
               B.prefix ? "" : " (if it is missing there, log out and back in)");
    }
    free(p.bin), free(p.desktop), free(p.svg), free(p.png), free(p.autostart), free(p.share);
    return ok;
}
