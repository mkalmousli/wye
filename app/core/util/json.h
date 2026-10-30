/*
 * Minimal read-only JSON access by dotted path ("assets.linux-x64.url").
 * Enough for update manifests; not a general-purpose parser.
 */
#ifndef WYE_JSON_H
#define WYE_JSON_H

#include <stdbool.h>
#include <stddef.h>

/* Pointer to the start of the value at path, or NULL. */
const char *wye_json_find(const char *json, const char *path);
/* Decodes a string value (handles \" \\ \n \t \/ escapes; \u becomes '?'). */
bool wye_json_string(const char *json, const char *path, char *out, size_t cap);
bool wye_json_number(const char *json, const char *path, double *out);

#endif
