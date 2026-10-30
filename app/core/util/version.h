#ifndef WYE_VERSION_H
#define WYE_VERSION_H

#include <stdbool.h>

/* <0 if a < b, 0 if equal, >0 if a > b. */
int wye_version_compare(const char *a, const char *b);

#endif
