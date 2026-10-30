/* Minimal test framework: TEST(name) defines a case, CHECK* assert. */
#ifndef WYE_TEST_H
#define WYE_TEST_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef void (*wye_test_fn)(void);

void wye_test_register(const char *name, wye_test_fn fn);
void wye_test_fail(const char *file, int line, const char *expr);

#if defined(_MSC_VER)
#error "tests use GCC/Clang constructor attributes"
/* test_env.c */
bool test_use_data_dir(const char *rel);

#endif

#define TEST(name)                                                        \
    static void name(void);                                               \
    __attribute__((constructor)) static void name##_reg(void)             \
    {                                                                     \
        wye_test_register(#name, name);                                   \
    }                                                                     \
    static void name(void)

#define CHECK(expr)                                                       \
    do {                                                                  \
        if (!(expr)) {                                                    \
            wye_test_fail(__FILE__, __LINE__, #expr);                     \
            return;                                                       \
        }                                                                 \
    } while (0)

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_MEM(a, b, n) CHECK(memcmp((a), (b), (n)) == 0)

/* test_env.c */
bool test_use_data_dir(const char *rel);
void test_isolate_home(void);  /* whole run: HOME is out/test-home */

#endif
