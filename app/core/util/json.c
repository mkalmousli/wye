#include "json.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char *ws(const char *p)
{
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

static const char *skip_string(const char *p)
{
    for (p++; *p && *p != '"'; ++p) {
        if (*p == '\\' && p[1]) {
            p++;
        }
    }
    return *p ? p + 1 : NULL;
}

/* Skips any value; returns the character after it, or NULL if malformed. */
static const char *skip_value(const char *p)
{
    p = ws(p);
    if (*p == '"') {
        return skip_string(p);
    }
    if (*p != '{' && *p != '[') {
        while (*p && !strchr(",}] \t\r\n", *p)) {
            p++;
        }
        return p;
    }
    int depth = 0;
    for (; *p; ++p) {
        if (*p == '"') {
            p = skip_string(p);
            if (!p) {
                return NULL;
            }
            p--;
        } else if (*p == '{' || *p == '[') {
            depth++;
        } else if ((*p == '}' || *p == ']') && --depth == 0) {
            return p + 1;
        }
    }
    return NULL;
}

/* Finds `key` (length n) among the members of the object at p. */
static const char *member(const char *p, const char *key, size_t n)
{
    p = ws(p);
    if (*p != '{') {
        return NULL;
    }
    for (p = ws(p + 1); p && *p == '"'; p = ws(p + 1)) {
        const char *k = p + 1, *end = skip_string(p);
        if (!end) {
            return NULL;
        }
        p = ws(end);
        if (*p != ':') {
            return NULL;
        }
        if ((size_t)(end - 1 - k) == n && strncmp(k, key, n) == 0) {
            return ws(p + 1);
        }
        p = skip_value(p + 1);
        if (!p || *(p = ws(p)) != ',') {
            return NULL;
        }
    }
    return NULL;
}

const char *wye_json_find(const char *json, const char *path)
{
    const char *p = json;
    while (p && *path) {
        size_t n = strcspn(path, ".");
        p = member(p, path, n);
        path += n + (path[n] == '.');
    }
    return p;
}

bool wye_json_string(const char *json, const char *path, char *out, size_t cap)
{
    const char *p = wye_json_find(json, path);
    size_t o = 0;
    if (!p || *p != '"' || cap == 0) {
        return false;
    }
    for (p++; *p && *p != '"' && o + 1 < cap; ++p) {
        char c = *p;
        if (c == '\\' && p[1]) {
            c = *++p;
            c = c == 'n' ? '\n' : c == 't' ? '\t' : c == 'u' ? '?' : c;
            if (*p == 'u') {
                p += strlen(p) >= 5 ? 4 : 0;
            }
        }
        out[o++] = c;
    }
    out[o] = '\0';
    return *p == '"';
}

bool wye_json_number(const char *json, const char *path, double *out)
{
    const char *p = wye_json_find(json, path);
    char *end;
    if (!p) {
        return false;
    }
    *out = strtod(p, &end);
    return end != p;
}
