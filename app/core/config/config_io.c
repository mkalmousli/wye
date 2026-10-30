/* Reading and writing the settings file (key=value lines, '#' comments). */
#include "config.h"

#include "log.h"
#include "plat_sys.h"
#include "util/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE "settings.ini"
#define CONFIG_MAX (64 * 1024)

static bool config_path(char *out, size_t len)
{
    char dir[PLAT_PATH_MAX];
    return plat_config_dir(dir, sizeof(dir)) && wye_path_join(out, len, dir, CONFIG_FILE);
}

static void apply(struct wye_config *c, const char *key, const char *value)
{
    int n;
    const struct wye_cfg_field *f = wye_config_fields(&n);
    for (int i = 0; i < n; ++i, ++f) {
        if (strcmp(f->key, key) != 0) {
            continue;
        }
        char *dst = (char *)c + f->offset;
        switch (f->type) {
        case WYE_CFG_INT: *(int *)dst = atoi(value); break;
        case WYE_CFG_BOOL: *(bool *)dst = strcmp(value, "true") == 0 || strcmp(value, "1") == 0; break;
        case WYE_CFG_I64: *(int64_t *)dst = strtoll(value, NULL, 10); break;
        case WYE_CFG_TEXT: wye_strcpy(dst, f->size, value); break;
        case WYE_CFG_HEX: wye_hex_decode(value, (uint8_t *)dst, f->size); break;
        }
        return;
    }
}

bool wye_config_load(struct wye_config *c)
{
    char path[PLAT_PATH_MAX];
    wye_config_defaults(c);
    char *data = config_path(path, sizeof(path)) ? (char *)wye_read_file(path, NULL, CONFIG_MAX) : NULL;
    if (!data) {
        return false;
    }
    for (char *line = strtok(data, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        char *eq = strchr(line, '=');
        line = wye_trim(line);
        if (*line == '#' || !eq) {
            continue;
        }
        *eq = '\0';
        apply(c, wye_trim(line), wye_trim(eq + 1));
    }
    free(data);
    wye_config_sanitize(c);
    return true;
}

static size_t format_field(const struct wye_config *c, const struct wye_cfg_field *f, char *out, size_t cap)
{
    const char *src = (const char *)c + f->offset;
    switch (f->type) {
    case WYE_CFG_INT: return (size_t)snprintf(out, cap, "%s=%d\n", f->key, *(const int *)src);
    case WYE_CFG_BOOL: return (size_t)snprintf(out, cap, "%s=%s\n", f->key, *(const bool *)src ? "true" : "false");
    case WYE_CFG_I64: return (size_t)snprintf(out, cap, "%s=%lld\n", f->key, (long long)*(const int64_t *)src);
    case WYE_CFG_HEX: {
        char hex[2 * 64 + 1];
        wye_hex_encode((const uint8_t *)src, f->size, hex, sizeof(hex));
        return (size_t)snprintf(out, cap, "%s=%s\n", f->key, hex);
    }
    default: return (size_t)snprintf(out, cap, "%s=%s\n", f->key, src);
    }
}

bool wye_config_save(const struct wye_config *c)
{
    char path[PLAT_PATH_MAX];
    static char buf[CONFIG_MAX];
    size_t used = (size_t)snprintf(buf, sizeof(buf), "# Wye settings\n");
    int n;
    const struct wye_cfg_field *f = wye_config_fields(&n);
    for (int i = 0; i < n && used < sizeof(buf); ++i) {
        used += format_field(c, &f[i], buf + used, sizeof(buf) - used);
    }
    if (used >= sizeof(buf) || !config_path(path, sizeof(path)) ||
        !wye_write_file_atomic(path, buf, used)) {
        LOG_E("config: failed to save settings");
        return false;
    }
    return true;
}
