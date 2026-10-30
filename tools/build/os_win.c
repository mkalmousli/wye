/* Windows primitives: files, directories, processes. */
#include "bld.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long long fs_mtime(const char *path)
{
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &a)) {
        return -1;
    }
    ULARGE_INTEGER t = {.LowPart = a.ftLastWriteTime.dwLowDateTime, .HighPart = a.ftLastWriteTime.dwHighDateTime};
    return (long long)t.QuadPart * 100; /* 100 ns ticks -> ns */
}

bool os_mkdir(const char *path)
{
    return CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool fs_rename(const char *from, const char *to)
{
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING) != 0;
}

void os_listdir(const char *dir, void (*fn)(void *ctx, const char *name, bool is_dir), void *ctx)
{
    WIN32_FIND_DATAA fd;
    char *pattern = str_fmt("%s\\*", dir);
    HANDLE h = FindFirstFileA(pattern, &fd);
    free(pattern);
    for (BOOL more = h != INVALID_HANDLE_VALUE; more; more = FindNextFileA(h, &fd)) {
        if (fd.cFileName[0] != '.') {
            fn(ctx, fd.cFileName, (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
        }
    }
    if (h != INVALID_HANDLE_VALUE) {
        FindClose(h);
    }
}

/* Quotes one argument the way the MSVC runtime parses it back. */
static void quote_arg(strlist *parts, const char *a)
{
    if (*a && !strpbrk(a, " \t\"")) {
        sl_push(parts, a);
        return;
    }
    char *q = malloc(strlen(a) * 2 + 3), *o = q;
    *o++ = '"';
    for (const char *p = a;; ++p) {
        size_t bs = 0;
        while (*p == '\\') {
            bs++, p++;
        }
        size_t reps = !*p ? bs * 2 : *p == '"' ? bs * 2 + 1 : bs;
        memset(o, '\\', reps);
        o += reps;
        if (!*p) {
            break;
        }
        *o++ = *p;
    }
    *o++ = '"';
    *o = '\0';
    sl_push(parts, q);
    free(q);
}

intptr_t proc_start(const strlist *argv)
{
    strlist parts = {0};
    size_t len = 1;
    for (size_t i = 0; i < argv->count; ++i) {
        quote_arg(&parts, argv->items[i]);
        len += strlen(parts.items[i]) + 1;
    }
    char *cmd = calloc(1, len);
    for (size_t i = 0; i < parts.count; ++i) {
        strcat(strcat(cmd, i ? " " : ""), parts.items[i]);
    }
    STARTUPINFOA si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi;
    fflush(stdout);
    BOOL ok = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    free(cmd);
    sl_free(&parts);
    if (!ok) {
        fprintf(stderr, "build: cannot run %s\n", argv->items[0]);
        return -1;
    }
    CloseHandle(pi.hThread);
    return (intptr_t)pi.hProcess;
}
