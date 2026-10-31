/* Points the app's per-user data directory at a scratch folder for one test. */
#define _DEFAULT_SOURCE /* realpath, setenv */
#include "test.h"

#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <sys/stat.h>
#endif

bool test_use_data_dir(const char *rel)
{
    char abs[1024];
#if defined(_WIN32)
    return _fullpath(abs, rel, sizeof(abs)) && _putenv_s("WYE_DATA_HOME", abs) == 0;
#else
    char *p = realpath(rel, NULL); /* XDG paths only count when absolute */
    bool ok = p && strlen(p) < sizeof(abs);
    if (ok) {
        strcpy(abs, p);
    }
    free(p);
    return ok && setenv("XDG_DATA_HOME", abs, 1) == 0;
#endif
}

/* For the whole run: a scratch home, so nothing lands in the user's real one (file manager bookmarks...). */
void test_isolate_home(void)
{
#if defined(_WIN32)
    _putenv_s("USERPROFILE", "out\\test-home");
    _putenv_s("WYE_CONFIG_HOME", "out\\test-config");
#else
    mkdir("out", 0755);
    mkdir("out/test-home", 0755);
    char *p = realpath("out/test-home", NULL);
    setenv("HOME", p ? p : "/nonexistent", 1);
    unsetenv("XDG_CONFIG_HOME");
    free(p);
#endif
}
