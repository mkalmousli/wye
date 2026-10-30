/* Internal launch modes and the per-OS process entry point. */
#include "app_entry.h"

#include "product.h"
#include "release/release_sign.h"

#include <stdio.h>
#include <string.h>

#if defined(__linux__)
#include "plat_linux.h"
#endif

static int help(void)
{
    printf("%s %s - open it from your app menu; everything is set up in its window.\n\n"
           "usage: wye [--background]\n"
           "  --background   start without showing the window (as at login)\n"
           "  --version      print the version\n"
           "  --help         this text\n\n%s\n",
           WYE_PRODUCT_NAME, WYE_VERSION_STRING, WYE_HOMEPAGE_URL);
    return 0;
}

int wye_special_mode(int argc, char **argv)
{
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        return help();
    }
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        return printf("%s %s\n", WYE_PRODUCT_NAME, WYE_VERSION_STRING), 0;
    }
#if defined(__linux__)
    if (argc > 1 && strcmp(argv[1], "--install-permissions") == 0) {
        return wye_linux_install_permissions();
    }
#endif
    if (argc > 3 && strcmp(argv[1], "--release-keygen") == 0) {
        char err[256] = "";
        bool ok = wye_release_keygen(argv[2], argv[3], err, sizeof(err));
        if (!ok) {
            fprintf(stderr, "key generation failed: %s\n", err);
        }
        return ok ? 0 : 1;
    }
    if (argc > 3 && strcmp(argv[1], "--release-sign") == 0) {
        uint8_t secret[64];
        char err[256] = "";
        bool ok = wye_release_load_secret(argv[2], secret) &&
                  wye_release_sign_file(secret, argv[3], err, sizeof(err));
        wye_release_wipe(secret);
        if (!ok) {
            fprintf(stderr, "signing failed: %s\n", err[0] ? err : "bad key file");
        }
        return ok ? 0 : 1;
    }
    return -1;
}

#if defined(_WIN32)
#include <stdlib.h>
#include <windows.h>
#include <shellapi.h>

/* GUI subsystem entry: convert the UTF-16 command line to UTF-8 argv. */
int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmd, int show)
{
    (void)inst; (void)prev; (void)cmd; (void)show;
    int argc = 0;
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **argv = calloc((size_t)argc + 1, sizeof(char *));
    for (int i = 0; i < argc; ++i) {
        int n = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL);
        argv[i] = malloc((size_t)n);
        WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], n, NULL, NULL);
    }
    LocalFree(wargv);
    return wye_main(argc, argv);
}
#else
int main(int argc, char **argv)
{
    return wye_main(argc, argv);
}
#endif
