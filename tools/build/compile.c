/* Compiling units of sources into objects and linking executables. */
#include "bld.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Records the command line; true when it differs from the previous build. */
static bool stamp_changed(const char *path, const strlist *flags)
{
    strlist lines = {0};
    size_t len = 0;
    for (size_t i = 0; i < flags->count; ++i) {
        sl_pushf(&lines, "%s\n", flags->items[i]);
        len += strlen(lines.items[i]);
    }
    char *text = calloc(1, len + 1);
    for (size_t i = 0; i < lines.count; ++i) {
        strcat(text, lines.items[i]);
    }
    char *old = fs_read(path, NULL);
    bool changed = !old || strcmp(old, text) != 0;
    fs_write_if_changed(path, text, len);
    free(old);
    free(text);
    sl_free(&lines);
    return changed;
}

bool unit_compile(unit *u)
{
    strlist stamp = {0};
    sl_push(&stamp, B.cc);
    sl_extend(&stamp, &u->cflags);
    char *stamp_path = str_fmt("%s/obj/%s.flags", B.out, u->name);
    bool force = stamp_changed(stamp_path, &stamp);
    strlist *cmds = calloc(u->srcs.count + 1, sizeof(strlist));
    char **labels = calloc(u->srcs.count + 1, sizeof(char *));
    size_t n = 0;
    for (size_t i = 0; i < u->srcs.count; ++i) {
        const char *src = u->srcs.items[i];
        char *obj = str_fmt("%s/obj/%s/%s.o", B.out, u->name, src);
        char *dep = str_fmt("%s.d", obj);
        sl_push(&u->objs, obj);
        if (needs_rebuild(obj, dep, force)) {
            fs_mkdir_parent(obj);
            sl_push(&cmds[n], B.cc);
            sl_extend(&cmds[n], &u->cflags);
            sl_pushn(&cmds[n], "-MMD", "-MF", dep, "-c", src, "-o", obj, (char *)NULL);
            labels[n++] = str_fmt("%s", src);
        }
        free(obj);
        free(dep);
    }
    bool ok = jobs_run(cmds, n, labels, "CC");
    if (!ok) {
        remove(stamp_path); /* make the next run retry everything in this unit */
    }
    for (size_t i = 0; i < n; ++i) {
        sl_free(&cmds[i]);
        free(labels[i]);
    }
    free(cmds);
    free(labels);
    free(stamp_path);
    sl_free(&stamp);
    return ok;
}

bool link_exe(const char *exe, const strlist *objs, const strlist *ldflags)
{
    strlist cmd = {0};
    sl_push(&cmd, B.cc);
    sl_extend(&cmd, objs);
    sl_pushn(&cmd, "-o", exe, (char *)NULL);
    sl_extend(&cmd, ldflags);
    const char *base = strrchr(exe, '/') ? strrchr(exe, '/') + 1 : exe;
    char *stamp_path = str_fmt("%s/obj/%s.link", B.out, base);
    bool changed = stamp_changed(stamp_path, &cmd);
    bool ok = true;
    if (changed || newer_than(exe, objs)) {
        fs_mkdir_parent(exe);
        echo_cmd("LINK", &cmd, exe);
        ok = run(&cmd);
        if (!ok) {
            remove(stamp_path);
        }
    }
    free(stamp_path);
    sl_free(&cmd);
    return ok;
}
