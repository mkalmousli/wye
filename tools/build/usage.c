/* ./build.c help */
#include "bld.h"

#include <stdio.h>

void usage(void)
{
    printf("Wye build tool %s\n\n"
           "usage: ./build.c [command...] [options]\n\n"
           "commands:\n"
           "  app               build the Wye app (default)      -> out/<os>/wye\n"
           "  tests             build and run the test suite         -> out/<os>/wye_tests\n"
           "  all               app + tests\n"
           "  firmware          build both Pico firmwares            -> out/firmware/*.uf2\n"
           "  clean             delete out/\n"
           "  install           build and install the desktop app for this user\n"
           "  uninstall         remove the installed desktop app\n"
           "  help              this text\n\n"
           "options:\n"
           "  --target=OS       linux, windows or macos (windows cross-builds with mingw-w64)\n"
           "  --debug           -O0 -g instead of -O2\n"
           "  --werror          treat warnings as errors\n"
           "  -jN, --jobs=N     parallel compiler jobs (default: CPU count)\n"
           "  --fw-dir=DIR      embed wye_*.uf2 from DIR (default out/firmware)\n"
           "  --pico-sdk=DIR    pico-sdk for 'firmware' (or set PICO_SDK_PATH)\n"
           "  --prefix=DIR      install root (Linux default ~/.local, e.g. /usr/local as root)\n"
           "  -v                print full compiler commands\n\n"
           "environment: CC (compiler for the target), BLD_CC (compiler that rebuilds this tool)\n",
           B.version[0] ? B.version : "");
}
