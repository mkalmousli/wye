#include "util.h"

#include "plat_sys.h"

#include <ctype.h>
#include <string.h>

bool wye_strcpy(char *dst, size_t cap, const char *src)
{
    if (cap == 0) {
        return false;
    }
    size_t n = strlen(src);
    bool fits = n < cap;
    if (!fits) {
        n = cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
    return fits;
}

bool wye_strcat(char *dst, size_t cap, const char *src)
{
    size_t used = strlen(dst);
    return used < cap && wye_strcpy(dst + used, cap - used, src);
}

bool wye_starts_with(const char *s, const char *prefix)
{
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

bool wye_ends_with(const char *s, const char *suffix)
{
    size_t a = strlen(s);
    size_t b = strlen(suffix);
    return a >= b && strcmp(s + a - b, suffix) == 0;
}

bool wye_contains_ci(const char *haystack, const char *needle)
{
    size_t n = strlen(needle);
    for (; *haystack; ++haystack) {
        size_t i = 0;
        while (i < n && haystack[i] &&
               tolower((unsigned char)haystack[i]) == tolower((unsigned char)needle[i])) {
            i++;
        }
        if (i == n) {
            return true;
        }
    }
    return n == 0;
}

char *wye_trim(char *s)
{
    while (isspace((unsigned char)*s)) {
        s++;
    }
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[--n] = '\0';
    }
    return s;
}

bool wye_path_join(char *dst, size_t cap, const char *a, const char *b)
{
    return wye_strcpy(dst, cap, a) && wye_strcat(dst, cap, plat_path_sep()) &&
           wye_strcat(dst, cap, b);
}
