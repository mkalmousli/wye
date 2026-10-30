/* Incremental builds: timestamps and the compiler's -MMD dependency files. */
#include "bld.h"

#include <stdlib.h>
#include <string.h>

bool newer_than(const char *out, const strlist *inputs)
{
    long long t = fs_mtime(out);
    for (size_t i = 0; t >= 0 && i < inputs->count; ++i) {
        long long in = fs_mtime(inputs->items[i]);
        if (in < 0 || in > t) {
            return true;
        }
    }
    return t < 0;
}

/* Next path in a make-style dependency list ("\ " escapes a space). */
static char *next_dep(char **cursor)
{
    char *p = *cursor;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ||
           (*p == '\\' && (p[1] == '\n' || p[1] == '\r'))) {
        p++;
    }
    if (!*p) {
        return NULL;
    }
    char *start = p, *o = p;
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        if (*p == '\\' && p[1] == ' ') {
            p++;
        } else if (*p == '$' && p[1] == '$') {
            p++;
        }
        *o++ = *p++;
    }
    *cursor = *p ? p + 1 : p;
    *o = '\0';
    return start;
}

/* True if obj is missing or older than anything it was built from. */
bool needs_rebuild(const char *obj, const char *depfile, bool force)
{
    long long t = fs_mtime(obj);
    char *deps = fs_read(depfile, NULL);
    char *colon = deps ? strstr(deps, ": ") : NULL;
    bool stale = force || t < 0 || !colon;
    char *cursor = colon ? colon + 2 : NULL;
    for (char *d = cursor ? next_dep(&cursor) : NULL; d && !stale; d = next_dep(&cursor)) {
        long long dt = fs_mtime(d);
        stale = dt < 0 || dt > t;
    }
    free(deps);
    return stale;
}
