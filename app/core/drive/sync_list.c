/* Folder snapshots: scanning, sorting, finding. */
#include "sync_list.h"
#include "fat_tree.h"
#include <stdlib.h>
#include <string.h>

void sync_list_add(struct sync_list *l, const char *path, bool dir, uint64_t size, int64_t here, int64_t there)
{
    if (l->n == l->cap) {
        size_t cap = l->cap ? l->cap * 2 : 64;
        struct sync_entry *grown = realloc(l->v, cap * sizeof(*grown));
        if (!grown) {
            return;
        }
        l->v = grown;
        l->cap = cap;
    }
    size_t len = strlen(path) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, path, len);
        l->v[l->n++] = (struct sync_entry){copy, dir, size, here, there};
    }
}

static void scan(const char *root, const char *rel, struct sync_list *out, int depth)
{
    struct tree_names names;
    char *dir = rel[0] ? tree_join(root, rel) : tree_join(root, ".");
    if (dir && depth < 32 && tree_list(dir, &names)) {
        for (size_t i = 0; i < names.count; ++i) {
            char *full = tree_join(dir, names.items[i]);
            char *sub = rel[0] ? tree_join(rel, names.items[i]) : tree_join(names.items[i], "");
            struct tree_item it;
            if (sub && !rel[0]) {
                sub[strlen(sub) - 1] = '\0'; /* tree_join added a trailing '/' */
            }
            if (full && sub && tree_stat(full, &it)) {
                sync_list_add(out, sub, it.is_dir, it.is_dir ? 0 : it.size, (int64_t)it.mtime, 0);
                if (it.is_dir) {
                    scan(root, sub, out, depth + 1);
                }
            }
            free(full);
            free(sub);
        }
        tree_free(&names);
    }
    free(dir);
}

static int by_path(const void *a, const void *b)
{
    return strcmp(((const struct sync_entry *)a)->path, ((const struct sync_entry *)b)->path);
}

void sync_list_sort(struct sync_list *l)
{
    if (l->n) {
        qsort(l->v, l->n, sizeof(*l->v), by_path);
    }
}

void sync_list_scan(const char *root, struct sync_list *out)
{
    memset(out, 0, sizeof(*out));
    scan(root, "", out, 0);
    sync_list_sort(out);
}

const struct sync_entry *sync_list_find(const struct sync_list *l, const char *path)
{
    struct sync_entry key = {(char *)path, false, 0, 0, 0};
    return l->n ? bsearch(&key, l->v, l->n, sizeof(*l->v), by_path) : NULL;
}

void sync_list_free(struct sync_list *l)
{
    for (size_t i = 0; i < l->n; ++i) {
        free(l->v[i].path);
    }
    free(l->v);
    memset(l, 0, sizeof(*l));
}
