/* macOS: per-user folders, the running executable, randomness and OS identity. */
#include "plat_sys.h"

#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <string.h>

static bool make_dir_under(NSString *base, char *out, size_t len)
{
    NSString *dir = [base stringByAppendingPathComponent:@"Wye"];
    if (![[NSFileManager defaultManager] createDirectoryAtPath:dir withIntermediateDirectories:YES
                                                    attributes:nil error:nil]) {
        return false;
    }
    return snprintf(out, len, "%s", dir.fileSystemRepresentation) < (int)len;
}

/* Tests and power users point these at another folder (same names as on Windows). */
static bool override_dir(const char *env, char *out, size_t len)
{
    const char *v = getenv(env);
    return v && *v && plat_mkdirs(v) && snprintf(out, len, "%s", v) < (int)len;
}

bool plat_config_dir(char *out, size_t len)
{
    if (override_dir("WYE_CONFIG_HOME", out, len)) {
        return true;
    }
    NSArray *dirs = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    return dirs.count && make_dir_under(dirs[0], out, len);
}

/* One folder for settings and the shared drive image. */
bool plat_data_dir(char *out, size_t len)
{
    return override_dir("WYE_DATA_HOME", out, len) || plat_config_dir(out, len);
}

bool plat_exe_path(char *out, size_t len)
{
    char raw[PLAT_PATH_MAX], real[PLAT_PATH_MAX];
    uint32_t n = sizeof(raw);
    if (_NSGetExecutablePath(raw, &n) != 0 || !realpath(raw, real)) {
        return false;
    }
    return snprintf(out, len, "%s", real) < (int)len;
}

bool plat_random(void *buf, size_t len)
{
    arc4random_buf(buf, len);
    return true;
}

#if defined(__aarch64__)
#define OS_ID "macos-arm64"
#else
#define OS_ID "macos-x64"
#endif

const char *plat_os_id(void)
{
    return OS_ID;
}

void plat_os_description(char *out, size_t len)
{
    NSOperatingSystemVersion v = [[NSProcessInfo processInfo] operatingSystemVersion];
    snprintf(out, len, "macOS %ld.%ld.%ld", (long)v.majorVersion, (long)v.minorVersion, (long)v.patchVersion);
}
