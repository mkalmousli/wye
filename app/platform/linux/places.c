/* Pinning a folder in file managers' sidebars (GTK bookmarks: Thunar, Nautilus, Nemo, Caja). */
#include "plat_sys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void file_uri(const char *path, char *out, size_t len)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t n = (size_t)snprintf(out, len, "file://");
    for (const unsigned char *p = (const unsigned char *)path; *p && n + 4 < len; ++p) {
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || strchr("/-_.~", *p)) {
            out[n++] = (char)*p;
        } else {
            out[n++] = '%';
            out[n++] = hex[*p >> 4];
            out[n++] = hex[*p & 15];
        }
    }
    out[n] = '\0';
}

void plat_places_pin(const char *path, const char *label)
{
    const char *home = getenv("HOME");
    char file[PLAT_PATH_MAX], uri[PLAT_PATH_MAX * 3], line[PLAT_PATH_MAX * 3 + 64];
    if (!home || path[0] != '/') {
        return; /* a relative folder (tests) cannot be a file:// place */
    }
    snprintf(file, sizeof(file), "%s/.config/gtk-3.0", home);
    plat_mkdirs(file);
    strncat(file, "/bookmarks", sizeof(file) - strlen(file) - 1);
    file_uri(path, uri, sizeof(uri));
    FILE *f = fopen(file, "r");
    size_t n = strlen(uri);
    while (f && fgets(line, sizeof(line), f)) {
        if (strncmp(line, uri, n) == 0 && (line[n] == ' ' || line[n] == '\n' || line[n] == '\0')) {
            fclose(f);
            return; /* already there (perhaps renamed by the user: keep their name) */
        }
    }
    if (f) {
        fclose(f);
    }
    if ((f = fopen(file, "a"))) {
        fprintf(f, "%s %s\n", uri, label);
        fclose(f);
    }
}
