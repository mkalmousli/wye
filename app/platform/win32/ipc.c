/* Local API transport: a named pipe that refuses remote clients. Its default
   security lets only this user, administrators and the system write to it. */
#include "plat_ipc.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

struct plat_ipc {
    HANDLE h;
    HANDLE ev;
    OVERLAPPED ov;      /* in use by the system while `pending`, so this struct never moves */
    bool pending;       /* an overlapped connect or read has not finished */
    plat_ipc *waiting;  /* listeners: the pipe instance waiting for the next client */
};

bool plat_ipc_name(char *out, size_t len)
{
    const char *env = getenv("WYE_API_SOCKET");
    char user[128] = "user";
    DWORD size = sizeof(user);
    GetUserNameA(user, &size);
    int n = env && *env ? snprintf(out, len, "%s", env) : snprintf(out, len, "\\\\.\\pipe\\wye-%s", user);
    return n > 0 && (size_t)n < len;
}

static plat_ipc *wrap(HANDLE h)
{
    plat_ipc *c = calloc(1, sizeof(*c));
    if (c) {
        c->h = h;
        c->ev = CreateEventA(NULL, TRUE, FALSE, NULL);
        c->ov.hEvent = c->ev;
    }
    return c;
}

/* The first instance also claims the name: a server someone else started makes this fail. */
static plat_ipc *instance(bool first)
{
    char name[256];
    if (!plat_ipc_name(name, sizeof(name))) {
        return NULL;
    }
    DWORD open = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | (first ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0);
    HANDLE h = CreateNamedPipeA(name, open, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                PIPE_UNLIMITED_INSTANCES, 16384, 16384, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    plat_ipc *c = wrap(h);
    if (c && !ConnectNamedPipe(h, &c->ov)) {
        DWORD e = GetLastError();
        if (e == ERROR_PIPE_CONNECTED) {
            SetEvent(c->ev);
        } else if (e == ERROR_IO_PENDING) {
            c->pending = true;
        } else {
            plat_ipc_close(c);
            return NULL;
        }
    }
    return c;
}

plat_ipc *plat_ipc_listen(void)
{
    plat_ipc *waiting = instance(true);
    plat_ipc *l = waiting ? calloc(1, sizeof(*l)) : NULL;
    if (l) {
        l->h = INVALID_HANDLE_VALUE;
        l->waiting = waiting;
    } else {
        plat_ipc_close(waiting);
    }
    return l;
}

plat_ipc *plat_ipc_accept(plat_ipc *l, int timeout_ms)
{
    if (!l->waiting) {
        l->waiting = instance(false); /* the last attempt failed */
        if (!l->waiting) {
            Sleep((DWORD)timeout_ms);
            return NULL;
        }
    }
    if (WaitForSingleObject(l->waiting->ev, (DWORD)timeout_ms) != WAIT_OBJECT_0) {
        return NULL;
    }
    plat_ipc *client = l->waiting;
    client->pending = false;
    l->waiting = instance(false);
    return client;
}

plat_ipc *plat_ipc_connect(void)
{
    char name[256];
    if (!plat_ipc_name(name, sizeof(name))) {
        return NULL;
    }
    for (int tries = 0; tries < 20; ++tries) {
        HANDLE h = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            return wrap(h);
        }
        if (GetLastError() != ERROR_PIPE_BUSY) { /* busy: the server is opening its next instance */
            return NULL;
        }
        WaitNamedPipeA(name, 100);
    }
    return NULL;
}

int plat_ipc_read(plat_ipc *c, void *buf, size_t cap, int timeout_ms)
{
    DWORD got = 0;
    if (!c->pending) {
        ResetEvent(c->ev);
        if (!ReadFile(c->h, buf, (DWORD)cap, &got, &c->ov) && GetLastError() != ERROR_IO_PENDING) {
            return -1;
        }
        c->pending = true; /* even a read done at once reports through the event */
    }
    if (WaitForSingleObject(c->ev, (DWORD)timeout_ms) != WAIT_OBJECT_0) {
        return 0; /* stays pending: a later call with the same buffer collects it */
    }
    c->pending = false;
    return GetOverlappedResult(c->h, &c->ov, &got, FALSE) && got > 0 ? (int)got : -1;
}

bool plat_ipc_write(plat_ipc *c, const void *data, size_t len)
{
    OVERLAPPED ov = {0};
    ov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    DWORD done = 0;
    bool ok = WriteFile(c->h, data, (DWORD)len, &done, &ov) ||
              (GetLastError() == ERROR_IO_PENDING && GetOverlappedResult(c->h, &ov, &done, TRUE));
    CloseHandle(ov.hEvent);
    return ok && done == len;
}

void plat_ipc_close(plat_ipc *c)
{
    if (!c) {
        return;
    }
    plat_ipc_close(c->waiting);
    if (c->h != INVALID_HANDLE_VALUE) {
        if (c->pending) { /* the system must let go of c->ov before it is freed */
            DWORD ignored;
            CancelIoEx(c->h, &c->ov);
            GetOverlappedResult(c->h, &c->ov, &ignored, TRUE);
        }
        CloseHandle(c->h);
    }
    if (c->ev) {
        CloseHandle(c->ev);
    }
    free(c);
}
