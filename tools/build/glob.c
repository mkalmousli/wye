/* Recursive, sorted file globbing on top of os_listdir. */
#include "bld.h"

#include <stdlib.h>
#include <string.h>

struct glob_ctx { const char *dir, *ext; bool recursive; strlist *out; };

static void glob_entry(void *p, const char *name, bool is_dir)
{
    struct glob_ctx *g = p;
    char *path = str_fmt("%s/%s", g->dir, name);
    size_t n = strlen(name), e = strlen(g->ext);
    if (is_dir && g->recursive) {
        fs_glob(path, g->ext, true, g->out);
    } else if (!is_dir && n > e && strcmp(name + n - e, g->ext) == 0) {
        sl_push(g->out, path);
    }
    free(path);
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/* Appends matching files under dir, sorted so builds are reproducible. */
void fs_glob(const char *dir, const char *ext, bool recursive, strlist *out)
{
    size_t first = out->count;
    struct glob_ctx g = {dir, ext, recursive, out};
    os_listdir(dir, glob_entry, &g);
    qsort(out->items + first, out->count - first, sizeof(char *), cmp_str);
}
