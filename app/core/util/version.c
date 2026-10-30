/* Semantic version comparison ("3.1.10" > "3.1.9"; "-beta" sorts before release). */
#include "version.h"

#include <stdlib.h>
#include <string.h>

static void parse(const char *v, long parts[3], bool *prerelease)
{
    char *end;
    if (*v == 'v' || *v == 'V') {
        v++;
    }
    for (int i = 0; i < 3; ++i) {
        parts[i] = strtol(v, &end, 10);
        v = *end == '.' ? end + 1 : end;
    }
    *prerelease = *end == '-';
}

int wye_version_compare(const char *a, const char *b)
{
    long pa[3], pb[3];
    bool ra, rb;
    parse(a, pa, &ra);
    parse(b, pb, &rb);
    for (int i = 0; i < 3; ++i) {
        if (pa[i] != pb[i]) {
            return pa[i] < pb[i] ? -1 : 1;
        }
    }
    if (ra != rb) {
        return ra ? -1 : 1;
    }
    return 0;
}
