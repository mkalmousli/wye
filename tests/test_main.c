#include "test.h"

#include <stdlib.h>

#define MAX_TESTS 512

static struct {
    const char *name;
    wye_test_fn fn;
} g_tests[MAX_TESTS];
static int g_count;
static int g_failed_current;

void wye_test_register(const char *name, wye_test_fn fn)
{
    if (g_count < MAX_TESTS) {
        g_tests[g_count].name = name;
        g_tests[g_count].fn = fn;
        g_count++;
    }
}

void wye_test_fail(const char *file, int line, const char *expr)
{
    fprintf(stderr, "    %s:%d: CHECK failed: %s\n", file, line, expr);
    g_failed_current = 1;
}

int main(int argc, char **argv)
{
    const char *filter = argc > 1 ? argv[1] : NULL;
    test_isolate_home();
    int failures = 0;
    int run = 0;
    for (int i = 0; i < g_count; ++i) {
        if (filter && !strstr(g_tests[i].name, filter)) {
            continue;
        }
        g_failed_current = 0;
        g_tests[i].fn();
        run++;
        if (g_failed_current) {
            failures++;
            fprintf(stderr, "FAIL %s\n", g_tests[i].name);
        }
    }
    printf("%d/%d tests passed\n", run - failures, run);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
