/* Small string, file and encoding helpers used across the app. */
#ifndef WYE_UTIL_H
#define WYE_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define WYE_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

/* Always NUL-terminates; returns false if truncated. */
bool wye_strcpy(char *dst, size_t cap, const char *src);
bool wye_strcat(char *dst, size_t cap, const char *src);
bool wye_starts_with(const char *s, const char *prefix);
bool wye_ends_with(const char *s, const char *suffix);
bool wye_contains_ci(const char *haystack, const char *needle);
char *wye_trim(char *s);
/* dst = a + sep + b */
bool wye_path_join(char *dst, size_t cap, const char *a, const char *b);

/* Reads a whole file (NUL-terminated); NULL on error or if larger than max. */
uint8_t *wye_read_file(const char *path, size_t *len, size_t max);
/* 64-bit file positions (drive images can exceed 2 GB, where long is 32-bit on Windows). */
bool wye_fseek64(FILE *f, uint64_t offset);
int64_t wye_ftell64(FILE *f);
/* Writes via temp file + rename so readers never see partial content. */
bool wye_write_file_atomic(const char *path, const void *data, size_t len);

size_t wye_hex_encode(const uint8_t *in, size_t len, char *out, size_t cap);
/* Returns bytes decoded, or (size_t)-1 on invalid input. */
size_t wye_hex_decode(const char *in, uint8_t *out, size_t cap);


#endif
