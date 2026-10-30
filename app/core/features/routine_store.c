/* Routine files on disk: location, listing, renaming, deleting. */
#include "routine.h"

#include "util/util.h"

#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

bool wye_routines_dir(char *out, size_t len)
{
    char data[PLAT_PATH_MAX];
    return plat_data_dir(data, sizeof(data)) && wye_path_join(out, len, data, "routines") &&
           plat_mkdirs(out);
}

void wye_routine_sanitize(const char *in, char *out, size_t len)
{
    size_t o = 0;
    for (; in && *in && o + 1 < len; ++in) {
        unsigned char c = (unsigned char)*in;
        if (isalnum(c) || c == '-' || c == '_') {
            out[o++] = (char)c;
        } else if ((c == ' ' || c == '.') && o > 0 && out[o - 1] != '-') {
            out[o++] = '-';
        }
    }
    while (o > 0 && out[o - 1] == '-') {
        o--;
    }
    out[o] = '\0';
}

static int newest_first(const void *a, const void *b)
{
    return strcmp(((const struct wye_routine_info *)b)->name, ((const struct wye_routine_info *)a)->name);
}

int wye_routine_list(struct wye_routine_info *out, int max)
{
    char dir[PLAT_PATH_MAX];
    DIR *d = wye_routines_dir(dir, sizeof(dir)) ? opendir(dir) : NULL;
    if (!d) {
        return 0;
    }
    int n = 0;
    for (struct dirent *e = readdir(d); e && n < max; e = readdir(d)) {
        if (!wye_ends_with(e->d_name, WYE_ROUTINE_EXT)) {
            continue;
        }
        wye_strcpy(out[n].name, sizeof(out[n].name), e->d_name);
        out[n].name[strlen(out[n].name) - strlen(WYE_ROUTINE_EXT)] = '\0';
        wye_path_join(out[n].path, sizeof(out[n].path), dir, e->d_name);
        n++;
    }
    closedir(d);
    qsort(out, (size_t)n, sizeof(*out), newest_first);
    return n;
}

bool wye_routine_rename(const char *path, const char *new_name, char *err, size_t errlen)
{
    char dir[PLAT_PATH_MAX], stem[WYE_ROUTINE_NAME_MAX], dest[PLAT_PATH_MAX];
    wye_routine_sanitize(new_name, stem, sizeof(stem));
    if (!stem[0]) {
        wye_strcpy(err, errlen, "Name cannot be empty");
        return false;
    }
    wye_strcat(stem, sizeof(stem), WYE_ROUTINE_EXT);
    if (!wye_routines_dir(dir, sizeof(dir)) || !wye_path_join(dest, sizeof(dest), dir, stem)) {
        wye_strcpy(err, errlen, "Path too long");
        return false;
    }
    if (plat_path_exists(dest)) {
        wye_strcpy(err, errlen, "A routine with that name already exists");
        return false;
    }
    if (!plat_rename_replace(path, dest)) {
        wye_strcpy(err, errlen, "Rename failed");
        return false;
    }
    return true;
}

bool wye_routine_delete(const char *path)
{
    return remove(path) == 0;
}
