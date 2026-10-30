/* The sync manifest on disk: one "<d|f> <size> <here> <there> <path>" line per entry. */
#include "sync_list.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool sync_list_load(const char *file, struct sync_list *out)
{
    memset(out, 0, sizeof(*out));
    FILE *f = fopen(file, "r");
    if (!f) {
        return false;
    }
    char line[2048], kind;
    uint64_t size;
    int64_t here, there;
    int used;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (sscanf(line, "%c %" SCNu64 " %" SCNd64 " %" SCNd64 " %n", &kind, &size, &here, &there, &used) == 4) {
            sync_list_add(out, line + used, kind == 'd', size, here, there);
        }
    }
    fclose(f);
    sync_list_sort(out);
    return true;
}

bool sync_list_save(const char *file, const struct sync_list *l)
{
    char tmp[1100];
    snprintf(tmp, sizeof(tmp), "%s.tmp", file);
    FILE *f = fopen(tmp, "w");
    if (!f) {
        return false;
    }
    for (size_t i = 0; i < l->n; ++i) {
        const struct sync_entry *e = &l->v[i];
        fprintf(f, "%c %" PRIu64 " %" PRId64 " %" PRId64 " %s\n", e->dir ? 'd' : 'f', e->size, e->here, e->there,
                e->path);
    }
    bool ok = fclose(f) == 0;
    if (ok) {
        remove(file);
        ok = rename(tmp, file) == 0;
    }
    return ok;
}
