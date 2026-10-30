/* Formatting and trimming helpers. */
#include "bld.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *str_vfmt(const char *fmt, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);
    char *s = malloc((size_t)n + 1);
    if (!s) {
        fprintf(stderr, "build: out of memory\n");
        exit(1);
    }
    vsnprintf(s, (size_t)n + 1, fmt, ap);
    return s;
}

char *str_fmt(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *s = str_vfmt(fmt, ap);
    va_end(ap);
    return s;
}

char *str_trim(char *s)
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
