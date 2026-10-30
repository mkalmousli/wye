/* Linux-only entry points used by main.c and the capture backend. */
#ifndef WYE_PLAT_LINUX_H
#define WYE_PLAT_LINUX_H

#include <stdbool.h>

/* Body of `wye --install-permissions` (runs as root via pkexec). */
int wye_linux_install_permissions(void);

#endif
