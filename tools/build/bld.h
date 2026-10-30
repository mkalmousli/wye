/* Wye build tool: shared declarations. Usage is described in build.c. */
#ifndef BLD_H
#define BLD_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Growable list of owned strings. */
typedef struct {
    char **items;
    size_t count, cap;
} strlist;

void sl_push(strlist *l, const char *s);
void sl_pushf(strlist *l, const char *fmt, ...);
void sl_pushn(strlist *l, ...);                  /* NULL-terminated */
void sl_extend(strlist *l, const strlist *src);
void sl_free(strlist *l);
char *str_fmt(const char *fmt, ...);             /* malloc'd */
char *str_vfmt(const char *fmt, va_list ap);
char *str_trim(char *s);

/* Files (util.c, os_*.c) */
long long fs_mtime(const char *path);            /* -1 if missing */
bool fs_exists(const char *path);
bool fs_mkdirs(const char *path);
bool fs_mkdir_parent(const char *file);
char *fs_read(const char *path, size_t *len);    /* NUL-terminated, malloc'd */
bool fs_write_if_changed(const char *path, const void *data, size_t len);
bool fs_copy(const char *from, const char *to);
bool fs_rename(const char *from, const char *to);
void fs_glob(const char *dir, const char *ext, bool recursive, strlist *out);
/* OS primitive behind fs_glob: calls fn for each entry (not . or ..). */
void os_listdir(const char *dir, void (*fn)(void *ctx, const char *name, bool is_dir), void *ctx);
bool os_mkdir(const char *path);

/* Processes (os_*.c). A handle is a pid (POSIX) or process HANDLE (Windows). */
intptr_t proc_start(const strlist *argv);        /* -1 on failure */
int proc_wait_any(const intptr_t *procs, int n, bool *ok);  /* index that finished */
bool run(const strlist *argv);                   /* start + wait, true on exit 0 */
char *run_capture(const char *cmd);              /* trimmed stdout, NULL on failure */
bool exec_self(const char *exe, char **argv);    /* replaces/forwards to a new binary */
int cpu_count(void);

/* Build context, filled from the command line (main.c, target.c). */
typedef enum { OS_LINUX, OS_WINDOWS, OS_MACOS } os_kind;

typedef struct {
    os_kind os;
    const char *os_name;       /* linux, windows, macos */
    const char *cc, *windres;
    const char *exe_ext;
    char out[64];              /* out/<os_name> */
    bool cross, debug, werror, verbose;
    int jobs;
    const char *fw_dir;        /* where wye_*.uf2 are embedded from */
    const char *pico_sdk;
    const char *prefix;        /* install root, NULL: per-user default */
    char version[32];
    int major, minor, patch;
} bld_ctx;

extern bld_ctx B;

void target_init(const char *os_name);           /* NULL: build for this machine */
void version_load(void);
char *pubkey_hex(const char *name);              /* keys/<name>.pub, "" if missing */
bool embed_file(const char *symbol, const char *input, char **out_c);

/* Compilation (compile.c, deps.c) */
typedef struct {
    const char *name;          /* object subdirectory and flags stamp */
    strlist srcs, cflags, objs;
} unit;

bool unit_compile(unit *u);
bool link_exe(const char *exe, const strlist *objs, const strlist *ldflags);
bool needs_rebuild(const char *obj, const char *depfile, bool force);
bool newer_than(const char *out, const strlist *inputs);
bool jobs_run(const strlist *cmds, size_t n, char *const *labels, const char *tag);
void echo_cmd(const char *tag, const strlist *argv, const char *label);

/* Build steps (step_*.c) */
void base_flags(strlist *f, bool warnings);      /* optimisation, OS defines, warnings */
void raylib_public_flags(strlist *f);
void miniaudio_flags(strlist *f);                 /* include path + trimmed feature set */
void raylib_libs(strlist *f);
bool step_third_party(strlist *objs, bool with_raylib);
void core_flags(strlist *f);
void core_libs(strlist *f);
bool step_core(strlist *objs);
bool step_app(void);
bool step_tests(bool run_them);
bool step_install(bool install);  /* false: uninstall */

/* Install helpers and per-OS installers (install_*.c) */
char *inst_home(const char *sub);                /* $HOME/sub, malloc'd */
bool inst_copy(const char *from, const char *to, bool executable);
bool inst_write(const char *path, const char *text);
void inst_remove(const char *path);
void inst_optional(const char *cmd);
bool install_linux(bool install);
bool install_macos(bool install);
bool install_win(bool install);
bool step_firmware(void);
void usage(void);
void rebuild_self(char **argv);
bool clean(void);

#endif
