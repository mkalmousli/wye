/*
 * Files and folders on Windows. The app manifest selects the UTF-8 code
 * page, so the ANSI ("A") APIs and fopen() take UTF-8 paths directly.
 */
#include "plat_sys.h"

#include <windows.h>
#include <shlobj.h>
#include <io.h>
#include <winioctl.h>
#include <stdio.h>
#include <string.h>

const char *plat_path_sep(void) { return "\\"; }

bool plat_path_exists(const char *path)
{
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

bool plat_rename_replace(const char *from, const char *to)
{
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;
}

bool plat_mkdirs(const char *path)
{
    char buf[PLAT_PATH_MAX];
    snprintf(buf, sizeof(buf), "%s", path);
    for (char *p = buf + 3; *p; ++p) { /* skip "C:\" */
        if (*p == '\\' || *p == '/') {
            char c = *p;
            *p = '\0';
            CreateDirectoryA(buf, NULL);
            *p = c;
        }
    }
    return CreateDirectoryA(buf, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static bool known_folder(int csidl, char *out, size_t len)
{
    char base[MAX_PATH];
    if (SHGetFolderPathA(NULL, csidl | CSIDL_FLAG_CREATE, NULL, 0, base) != S_OK) {
        return false;
    }
    int n = snprintf(out, len, "%s\\Wye", base);
    return n > 0 && (size_t)n < len && plat_mkdirs(out);
}

bool plat_config_dir(char *out, size_t len) { return known_folder(CSIDL_APPDATA, out, len); }
bool plat_data_dir(char *out, size_t len) { return known_folder(CSIDL_LOCAL_APPDATA, out, len); }

bool plat_temp_dir(char *out, size_t len)
{
    char base[MAX_PATH];
    DWORD n = GetTempPathA(sizeof(base), base);
    return n > 0 && (size_t)snprintf(out, len, "%sWye", base) < len && plat_mkdirs(out);
}

bool plat_exe_path(char *out, size_t len)
{
    DWORD n = GetModuleFileNameA(NULL, out, (DWORD)len);
    return n > 0 && n < len;
}

int plat_uf2_drives(char paths[][PLAT_PATH_MAX], int max)
{
    DWORD mask = GetLogicalDrives();
    int n = 0;
    for (int d = 0; d < 26 && n < max; ++d) {
        char info[16];
        snprintf(info, sizeof(info), "%c:\\INFO_UF2.TXT", 'A' + d);
        if ((mask & (1u << d)) && GetDriveTypeA((char[]){(char)('A' + d), ':', '\\', 0}) == DRIVE_REMOVABLE &&
            plat_path_exists(info)) {
            snprintf(paths[n++], PLAT_PATH_MAX, "%c:\\", 'A' + d);
        }
    }
    return n;
}

bool plat_file_sparse(FILE *f)
{
    DWORD out = 0;
    HANDLE h = (HANDLE)_get_osfhandle(_fileno(f));
    return h != INVALID_HANDLE_VALUE && DeviceIoControl(h, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &out, NULL);
}
