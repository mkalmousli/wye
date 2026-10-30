/* Command line, self-rebuild and dispatch. */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool command(const char *c)
{
    if (strcmp(c, "app") == 0) return step_app();
    if (strcmp(c, "tests") == 0) return step_tests(true);
    if (strcmp(c, "all") == 0) return step_app() && step_tests(true);
    if (strcmp(c, "firmware") == 0) return step_firmware();
    if (strcmp(c, "clean") == 0) return clean();
    if (strcmp(c, "install") == 0) return step_app() && step_install(true);
    if (strcmp(c, "uninstall") == 0) return step_install(false);
    if (strcmp(c, "help") == 0) return usage(), true;
    fprintf(stderr, "build: unknown command '%s' (try ./build.c help)\n", c);
    return false;
}

static const char *opt(const char *arg, const char *name)
{
    size_t n = strlen(name);
    return strncmp(arg, name, n) == 0 ? arg + n : NULL;
}

int main(int argc, char **argv)
{
    const char *target = NULL, *v;
    strlist cmds = {0};
    B.jobs = cpu_count();
    B.fw_dir = "out/firmware";
    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if ((v = opt(a, "--target="))) target = v;
        else if ((v = opt(a, "--fw-dir="))) B.fw_dir = v;
        else if ((v = opt(a, "--pico-sdk="))) B.pico_sdk = v;
        else if ((v = opt(a, "--jobs="))) B.jobs = atoi(v);
        else if ((v = opt(a, "-j")) && *v) B.jobs = atoi(v);
        else if (strcmp(a, "--debug") == 0) B.debug = true;
        else if (strcmp(a, "--werror") == 0) B.werror = true;
        else if (strcmp(a, "-v") == 0) B.verbose = true;
        else if ((v = opt(a, "--prefix="))) B.prefix = v;
        else if (a[0] == '-') return fprintf(stderr, "build: unknown option %s\n", a), 2;
        else sl_push(&cmds, a);
    }
    target_init(NULL);
    rebuild_self(argv);
    target_init(target);
    version_load();
    if (cmds.count == 0) {
        sl_push(&cmds, "app");
    }
    for (size_t i = 0; i < cmds.count; ++i) {
        if (!command(cmds.items[i])) {
            fprintf(stderr, "build: %s failed\n", cmds.items[i]);
            return 1;
        }
    }
    return 0;
}
