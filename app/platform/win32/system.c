/* System identification, randomness, URLs, start-at-login and permissions. */
#include "plat_sys.h"

#include <windows.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <stdio.h>

#define RUN_KEY "Software\\Microsoft\\Windows\\CurrentVersion\\Run"

const char *plat_os_id(void)
{
#if defined(_M_ARM64) || defined(__aarch64__)
    return "windows-arm64";
#else
    return "windows-x64";
#endif
}

void plat_os_description(char *out, size_t len)
{
    typedef LONG(WINAPI * rtl_get_version)(OSVERSIONINFOW *);
    OSVERSIONINFOW v = {.dwOSVersionInfoSize = sizeof(v)};
    rtl_get_version fn = (rtl_get_version)(void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
    if (fn) {
        fn(&v); /* GetVersionEx lies to unmanifested apps; ntdll does not */
    }
    snprintf(out, len, "Windows %lu.%lu build %lu", v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber);
}

bool plat_random(void *buf, size_t len)
{
    return BCryptGenRandom(NULL, buf, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
}

bool plat_open_url(const char *url)
{
    return (INT_PTR)ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL) > 32;
}

bool plat_reveal_path(const char *path)
{
    return plat_open_url(path);
}

bool plat_autostart_get(void)
{
    char buf[PLAT_PATH_MAX];
    DWORD size = sizeof(buf);
    return RegGetValueA(HKEY_CURRENT_USER, RUN_KEY, "Wye", RRF_RT_REG_SZ, NULL, buf, &size) == ERROR_SUCCESS;
}

bool plat_autostart_set(bool enable)
{
    HKEY key;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }
    LSTATUS rc;
    if (enable) {
        char exe[PLAT_PATH_MAX], cmd[PLAT_PATH_MAX + 32];
        plat_exe_path(exe, sizeof(exe));
        snprintf(cmd, sizeof(cmd), "\"%s\" --background", exe);
        rc = RegSetValueExA(key, "Wye", 0, REG_SZ, (const BYTE *)cmd, (DWORD)strlen(cmd) + 1);
    } else {
        rc = RegDeleteValueA(key, "Wye");
        rc = rc == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : rc;
    }
    RegCloseKey(key);
    return rc == ERROR_SUCCESS;
}

/* Low-level hooks need no special permission on Windows. */
enum plat_permission plat_input_permission(void) { return PLAT_PERM_OK; }

bool plat_input_permission_request(char *err, size_t errlen)
{
    (void)err;
    (void)errlen;
    return true;
}

void plat_audio_full_volume(const char *id, bool capture)
{
    (void)id; /* Windows opens new USB sound devices at full volume */
    (void)capture;
}

void plat_places_pin(const char *path, const char *label)
{
    (void)path; /* the folder already sits in the user's profile, next to Documents */
    (void)label;
}
