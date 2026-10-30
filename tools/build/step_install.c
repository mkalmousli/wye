/* ./build.c install | uninstall: the desktop app for the current user. */
#include "bld.h"

#include <stdio.h>

bool step_install(bool install)
{
    if (B.cross) {
        fprintf(stderr, "build: install only works for this machine (drop --target)\n");
        return false;
    }
    switch (B.os) {
    case OS_WINDOWS: return install_win(install);
    case OS_MACOS: return install_macos(install);
    default: return install_linux(install);
    }
}
