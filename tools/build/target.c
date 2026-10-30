/* Target platform, toolchain, version and key inputs. */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bld_ctx B;

#if defined(_WIN32)
#define HOST_OS "windows"
#elif defined(__APPLE__)
#define HOST_OS "macos"
#else
#define HOST_OS "linux"
#endif

void target_init(const char *os_name)
{
    const char *name = os_name ? os_name : HOST_OS;
    B.cross = strcmp(name, HOST_OS) != 0;
    B.os = strcmp(name, "windows") == 0 ? OS_WINDOWS : strcmp(name, "macos") == 0 ? OS_MACOS : OS_LINUX;
    B.os_name = B.os == OS_WINDOWS ? "windows" : B.os == OS_MACOS ? "macos" : "linux";
    B.exe_ext = B.os == OS_WINDOWS ? ".exe" : "";
    snprintf(B.out, sizeof(B.out), "out/%s", B.os_name);
    const char *cc = getenv("CC");
    if (B.cross && B.os == OS_WINDOWS) {
        B.cc = cc ? cc : "x86_64-w64-mingw32-gcc";
        B.windres = "x86_64-w64-mingw32-windres";
    } else if (B.cross) {
        fprintf(stderr, "build: cannot cross-compile for %s from %s; build on that system\n", name, HOST_OS);
        exit(1);
    } else {
        B.cc = cc ? cc : (B.os == OS_WINDOWS ? "gcc" : "cc");
        B.windres = "windres";
    }
}

void version_load(void)
{
    char *text = fs_read("VERSION", NULL);
    snprintf(B.version, sizeof(B.version), "%s", text ? str_trim(text) : "0.0.0");
    free(text);
    if (sscanf(B.version, "%d.%d.%d", &B.major, &B.minor, &B.patch) != 3) {
        fprintf(stderr, "build: VERSION must look like 1.2.3 (got '%s')\n", B.version);
        exit(1);
    }
}

char *pubkey_hex(const char *name)
{
    char *path = str_fmt("keys/%s.pub", name);
    char *text = fs_read(path, NULL);
    if (!text) {
        fprintf(stderr, "build: warning: %s missing - %s verification disabled in this build\n", path, name);
    }
    char *hex = str_fmt("%s", text ? str_trim(text) : "");
    free(text);
    free(path);
    return hex;
}

/* Writes <out>/gen/<symbol>.c defining symbol[] and symbol_len (0 if input is missing). */
bool embed_file(const char *symbol, const char *input, char **out_c)
{
    size_t len = 0;
    unsigned char *data = input ? (unsigned char *)fs_read(input, &len) : NULL;
    if (!data) {
        fprintf(stderr, "build: warning: %s not found - building without it\n", input ? input : symbol);
        len = 0;
    }
    char *head = str_fmt("/* Generated from %s by build.c; do not edit. */\n#include <stddef.h>\n"
                         "const unsigned char %s[] = {\n", data ? input : "(nothing)", symbol);
    size_t hlen = strlen(head), cap = hlen + len * 5 + len / 16 + 128;
    char *src = malloc(cap);
    size_t o = (size_t)snprintf(src, cap, "%s", head);
    for (size_t i = 0; i < len; ++i) {
        o += (size_t)snprintf(src + o, cap - o, "%u,%s", data[i], i % 24 == 23 ? "\n" : "");
    }
    o += (size_t)snprintf(src + o, cap - o, "%s};\nconst size_t %s_len = %zu;\n", len ? "" : "0", symbol, len);
    *out_c = str_fmt("%s/gen/%s.c", B.out, symbol);
    bool ok = fs_write_if_changed(*out_c, src, o);
    free(head);
    free(src);
    free(data);
    return ok;
}
