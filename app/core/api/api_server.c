/* The API listener: one thread per client, commands run one at a time. */
#include "api.h"

#include "log.h"
#include "plat_ipc.h"
#include "plat_thread.h"

#include <stdlib.h>
#include <string.h>

#define MAX_CLIENTS 8
#define POLL_MS 200

struct client {
    plat_ipc *conn;
    plat_thread *thread;
    volatile bool done;
};

static struct {
    const struct wye_api_ops *ops;
    plat_ipc *listener;
    plat_thread *thread;
    plat_mutex *lock;
    volatile bool stop;
    struct client clients[MAX_CLIENTS];
} g_api;

static void run_line(struct client *c, char *line)
{
    static char reply[512];
    plat_mutex_lock(g_api.lock);
    wye_api_command(g_api.ops, line, reply, sizeof(reply) - 1, &g_api.stop);
    strcat(reply, "\n");
    bool ok = plat_ipc_write(c->conn, reply, strlen(reply));
    plat_mutex_unlock(g_api.lock);
    c->done = c->done || !ok;
}

static void client_thread(void *arg)
{
    struct client *c = arg;
    char *line = malloc(WYE_API_LINE);
    size_t used = 0;
    bool too_long = false;
    while (line && !g_api.stop && !c->done) {
        int n = plat_ipc_read(c->conn, line + used, WYE_API_LINE - 1 - used, POLL_MS);
        if (n < 0) {
            break;
        }
        used += (size_t)n;
        line[used] = '\0';
        char *nl;
        while (!c->done && (nl = strchr(line, '\n')) != NULL) {
            size_t len = (size_t)(nl - line);
            *nl = '\0';
            if (len && line[len - 1] == '\r') {
                line[len - 1] = '\0';
            }
            if (too_long) {
                plat_ipc_write(c->conn, "err bad-request line too long\n", 30);
                too_long = false;
            } else {
                run_line(c, line);
            }
            used -= len + 1;
            memmove(line, nl + 1, used + 1);
        }
        if (used == WYE_API_LINE - 1) { /* no newline in a whole buffer: drop up to the next one */
            too_long = true;
            used = 0;
        }
    }
    free(line);
    c->done = true;
}

static void reap(bool all)
{
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        struct client *c = &g_api.clients[i];
        if (c->conn && (c->done || all)) {
            plat_thread_join(c->thread);
            plat_ipc_close(c->conn);
            memset(c, 0, sizeof(*c));
        }
    }
}

static void listen_thread(void *arg)
{
    (void)arg;
    while (!g_api.stop) {
        plat_ipc *conn = plat_ipc_accept(g_api.listener, POLL_MS);
        reap(false);
        if (!conn) {
            continue;
        }
        struct client *c = NULL;
        for (int i = 0; i < MAX_CLIENTS && !c; ++i) {
            c = g_api.clients[i].conn ? NULL : &g_api.clients[i];
        }
        if (!c) {
            plat_ipc_write(conn, "err busy too many clients\n", 26);
            plat_ipc_close(conn);
            continue;
        }
        c->conn = conn;
        c->done = false;
        c->thread = plat_thread_start(client_thread, c);
        if (!c->thread) {
            plat_ipc_close(conn);
            memset(c, 0, sizeof(*c));
        }
    }
    reap(true);
}

bool wye_api_start(const struct wye_api_ops *ops)
{
    if (g_api.listener) {
        return true;
    }
    g_api.listener = plat_ipc_listen();
    if (!g_api.listener) {
        LOG_W("api: cannot open the local socket; other apps cannot reach Wye");
        return false;
    }
    if (!g_api.lock) {
        g_api.lock = plat_mutex_new();
    }
    g_api.ops = ops;
    g_api.stop = false;
    g_api.thread = plat_thread_start(listen_thread, NULL);
    if (!g_api.thread) {
        plat_ipc_close(g_api.listener);
        g_api.listener = NULL;
        return false;
    }
    LOG_I("api: listening for local apps");
    return true;
}

void wye_api_stop(void)
{
    if (!g_api.listener) {
        return;
    }
    g_api.stop = true;
    plat_thread_join(g_api.thread);
    plat_ipc_close(g_api.listener);
    g_api.listener = NULL;
    g_api.thread = NULL;
}
