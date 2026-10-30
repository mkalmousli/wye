/* Local API transport: a Unix socket only this user can reach. */
#define _GNU_SOURCE /* struct ucred, SOCK_CLOEXEC, lstat */
#include "plat_ipc.h"

#include "plat_sys.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#if !defined(SOCK_CLOEXEC) /* macOS */
#define SOCK_CLOEXEC 0
#endif
#if !defined(MSG_NOSIGNAL) /* macOS: the app ignores SIGPIPE */
#define MSG_NOSIGNAL 0
#endif

struct plat_ipc {
    int fd;
    char path[sizeof(((struct sockaddr_un *)0)->sun_path)]; /* listeners only: removed on close */
};

/* No XDG_RUNTIME_DIR: a private directory of our own, not one someone else prepared. */
static bool private_temp_dir(char *out, size_t len)
{
    struct stat st;
    if (!plat_temp_dir(out, len) || lstat(out, &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode) && st.st_uid == getuid() && chmod(out, 0700) == 0;
}

bool plat_ipc_name(char *out, size_t len)
{
    const char *env = getenv("WYE_API_SOCKET"), *rt = getenv("XDG_RUNTIME_DIR");
    char dir[PLAT_PATH_MAX];
    int n;
    if (env && *env) {
        n = snprintf(out, len, "%s", env);
    } else if (rt && *rt) {
        n = snprintf(out, len, "%s/wye.sock", rt);
    } else if (private_temp_dir(dir, sizeof(dir))) {
        n = snprintf(out, len, "%s/wye.sock", dir);
    } else {
        return false;
    }
    return n > 0 && (size_t)n < len;
}

static bool address(struct sockaddr_un *a)
{
    memset(a, 0, sizeof(*a));
    a->sun_family = AF_UNIX;
    return plat_ipc_name(a->sun_path, sizeof(a->sun_path));
}

static plat_ipc *wrap(int fd)
{
    plat_ipc *c = calloc(1, sizeof(*c));
    if (!c) {
        close(fd);
        return NULL;
    }
    c->fd = fd;
    return c;
}

plat_ipc *plat_ipc_listen(void)
{
    struct sockaddr_un a;
    int fd = address(&a) ? socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0) : -1;
    if (fd < 0) {
        return NULL;
    }
    unlink(a.sun_path); /* left by a crash: the single-instance lock says it is not in use */
    mode_t old = umask(0177);
    bool ok = bind(fd, (struct sockaddr *)&a, sizeof(a)) == 0;
    umask(old);
    if (!ok || listen(fd, 8) != 0) {
        close(fd);
        return NULL;
    }
    plat_ipc *c = wrap(fd);
    if (c) {
        memcpy(c->path, a.sun_path, sizeof(c->path));
    }
    return c;
}

static bool same_user(int fd)
{
#if defined(__linux__)
    struct ucred who;
    socklen_t len = sizeof(who);
    return getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &who, &len) == 0 && who.uid == getuid();
#elif defined(__APPLE__)
    uid_t uid;
    gid_t gid;
    return getpeereid(fd, &uid, &gid) == 0 && uid == getuid();
#else
    (void)fd;
    return true; /* the socket file's 0600 still applies */
#endif
}

plat_ipc *plat_ipc_accept(plat_ipc *listener, int timeout_ms)
{
    struct pollfd p = {listener->fd, POLLIN, 0};
    if (poll(&p, 1, timeout_ms) <= 0) {
        return NULL;
    }
    int fd = accept(listener->fd, NULL, NULL);
    if (fd < 0) {
        return NULL;
    }
    if (!same_user(fd)) {
        close(fd);
        return NULL;
    }
    return wrap(fd);
}

plat_ipc *plat_ipc_connect(void)
{
    struct sockaddr_un a;
    int fd = address(&a) ? socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0) : -1;
    if (fd < 0) {
        return NULL;
    }
    if (connect(fd, (struct sockaddr *)&a, sizeof(a)) != 0) {
        close(fd);
        return NULL;
    }
    return wrap(fd);
}

int plat_ipc_read(plat_ipc *c, void *buf, size_t cap, int timeout_ms)
{
    struct pollfd p = {c->fd, POLLIN, 0};
    int ready = poll(&p, 1, timeout_ms);
    if (ready == 0 || (ready < 0 && errno == EINTR)) {
        return 0;
    }
    ssize_t n = ready > 0 ? read(c->fd, buf, cap) : -1;
    if (n < 0 && (errno == EINTR || errno == EAGAIN)) {
        return 0;
    }
    return n > 0 ? (int)n : -1;
}

bool plat_ipc_write(plat_ipc *c, const void *data, size_t len)
{
    const char *p = data;
    while (len > 0) {
        ssize_t n = send(c->fd, p, len, MSG_NOSIGNAL);
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n <= 0) {
            return false;
        }
        p += n;
        len -= (size_t)n;
    }
    return true;
}

void plat_ipc_close(plat_ipc *c)
{
    if (c) {
        close(c->fd);
        if (c->path[0]) {
            unlink(c->path);
        }
        free(c);
    }
}
