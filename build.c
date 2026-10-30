//usr/bin/env true; cd "$(dirname "$0")" || exit 1; t=out/.tool/build; [ -x "$t" ] || { mkdir -p out/.tool && echo "build: compiling the build tool" && ${BLD_CC:-cc} -O1 -o "$t" build.c; } || exit 1; exec "$t" "$@"
/*
 * Wye build tool - plain C, no make, no CMake. This file is both the
 * tool's source and its launcher: the first line is a shell command (and a C
 * comment) that compiles it to out/.tool/build on first use and runs it.
 * Afterwards it rebuilds itself whenever this file or tools/build/ changes.
 *
 *   ./build.c                   build the app            -> out/<os>/wye
 *   ./build.c tests             build and run the tests
 *   ./build.c firmware          build both Pico firmwares (needs the pico-sdk)
 *   ./build.c install           build and install the desktop app (per user)
 *   ./build.c uninstall         remove it again
 *   ./build.c --target=windows  cross-build Wye.exe with mingw-w64
 *   ./build.c help              every command and option
 *
 * Windows without a POSIX shell: gcc -o build.exe build.c && build.exe
 */
#if !defined(_WIN32)
#define _DEFAULT_SOURCE /* POSIX + BSD APIs even under -std=c11 */
#endif
#include "tools/build/str.c"
#include "tools/build/strlist.c"
#include "tools/build/files.c"
#include "tools/build/glob.c"
#if defined(_WIN32)
#include "tools/build/os_win.c"
#include "tools/build/os_win_proc.c"
#else
#include "tools/build/os_posix.c"
#include "tools/build/os_posix_proc.c"
#endif
#include "tools/build/jobs.c"
#include "tools/build/deps.c"
#include "tools/build/compile.c"
#include "tools/build/target.c"
#include "tools/build/step_third_party.c"
#include "tools/build/step_core.c"
#include "tools/build/step_app.c"
#include "tools/build/step_tests.c"
#include "tools/build/step_firmware.c"
#include "tools/build/install_files.c"
#include "tools/build/install_linux.c"
#include "tools/build/install_macos.c"
#include "tools/build/install_win.c"
#include "tools/build/step_install.c"
#include "tools/build/usage.c"
#include "tools/build/self.c"
#include "tools/build/main.c"
