/* Local-only connections between processes of this user: a Unix socket or a named pipe. */
#ifndef WYE_PLAT_IPC_H
#define WYE_PLAT_IPC_H

#include <stdbool.h>
#include <stddef.h>

typedef struct plat_ipc plat_ipc;

/* Where the API listens: the WYE_API_SOCKET environment variable, or a per-user default. */
bool plat_ipc_name(char *out, size_t len);
plat_ipc *plat_ipc_listen(void);
/* A new client, or NULL if none came within timeout_ms. */
plat_ipc *plat_ipc_accept(plat_ipc *listener, int timeout_ms);
plat_ipc *plat_ipc_connect(void);
/* Bytes read; 0 on timeout; -1 once the other side is gone. */
int plat_ipc_read(plat_ipc *c, void *buf, size_t cap, int timeout_ms);
bool plat_ipc_write(plat_ipc *c, const void *data, size_t len);
void plat_ipc_close(plat_ipc *c);

#endif
