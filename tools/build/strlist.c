/* String lists and formatting helpers. */
#include "bld.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_str(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (!d) {
        fprintf(stderr, "build: out of memory\n");
        exit(1);
    }
    return memcpy(d, s, n);
}

void sl_push(strlist *l, const char *s)
{
    if (l->count + 1 >= l->cap) {
        l->cap = l->cap ? l->cap * 2 : 16;
        l->items = realloc(l->items, l->cap * sizeof(char *));
        if (!l->items) {
            fprintf(stderr, "build: out of memory\n");
            exit(1);
        }
    }
    l->items[l->count++] = dup_str(s);
    l->items[l->count] = NULL; /* always usable as an argv */
}

void sl_pushf(strlist *l, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *s = str_vfmt(fmt, ap);
    va_end(ap);
    sl_push(l, s);
    free(s);
}

void sl_pushn(strlist *l, ...)
{
    va_list ap;
    va_start(ap, l);
    for (const char *s = va_arg(ap, const char *); s; s = va_arg(ap, const char *)) {
        sl_push(l, s);
    }
    va_end(ap);
}

void sl_extend(strlist *l, const strlist *src)
{
    for (size_t i = 0; i < src->count; ++i) {
        sl_push(l, src->items[i]);
    }
}

void sl_free(strlist *l)
{
    for (size_t i = 0; i < l->count; ++i) {
        free(l->items[i]);
    }
    free(l->items);
    *l = (strlist){0};
}
