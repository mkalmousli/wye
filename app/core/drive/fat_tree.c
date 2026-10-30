/* Walking real folders for the FAT packer. */
#include "fat_tree.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char *tree_join(const char *dir, const char *name)
{
    size_t a = strlen(dir), b = strlen(name);
    char *s = malloc(a + b + 2);
    if (s) {
        memcpy(s, dir, a);
        s[a] = '/'; /* Windows accepts '/' too */
        memcpy(s + a + 1, name, b + 1);
    }
    return s;
}

static char *copy_name(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    return d ? memcpy(d, s, n) : NULL;
}

bool tree_list(const char *path, struct tree_names *out)
{
    DIR *d = opendir(path);
    size_t cap = 0;
    memset(out, 0, sizeof(*out));
    for (struct dirent *e = d ? readdir(d) : NULL; e; e = readdir(d)) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
            continue;
        }
        if (out->count == cap) {
            cap = cap ? cap * 2 : 32;
            char **grown = realloc(out->items, cap * sizeof(char *));
            if (!grown) {
                break;
            }
            out->items = grown;
        }
        out->items[out->count++] = copy_name(e->d_name);
    }
    if (d) {
        closedir(d);
    }
    return d != NULL;
}

void tree_free(struct tree_names *n)
{
    for (size_t i = 0; i < n->count; ++i) {
        free(n->items[i]);
    }
    free(n->items);
    memset(n, 0, sizeof(*n));
}

bool tree_stat(const char *path, struct tree_item *out)
{
    struct stat st;
    if (stat(path, &st) != 0 || !(S_ISDIR(st.st_mode) || S_ISREG(st.st_mode))) {
        return false;
    }
    out->is_dir = S_ISDIR(st.st_mode);
    out->size = (uint64_t)st.st_size;
    out->mtime = st.st_mtime;
    return true;
}
