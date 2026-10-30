/* `wye status`, `wye type hello`: the API from a shell. Talks to the running app. */
#include "api.h"

#include "plat_ipc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPLY_TIMEOUT_MS 300000 /* typing a long text takes a while */

static bool build_line(int argc, char **argv, char *out, size_t cap)
{
    size_t used = (size_t)snprintf(out, cap, "%s", argv[1]);
    static char escaped[WYE_API_LINE];
    bool type = strcmp(argv[1], "type") == 0;
    for (int i = 2; i < argc; ++i) {
        if (type) {
            wye_api_escape(argv[i], escaped, sizeof(escaped));
        }
        int n = snprintf(out + used, cap - used, "%s%s", used && (!type || i >= 2) ? " " : "", type ? escaped : argv[i]);
        if (n < 0 || (size_t)n >= cap - used) {
            return false;
        }
        used += (size_t)n;
    }
    return used + 2 < cap;
}

bool wye_api_cli(int argc, char **argv, int *exit_code)
{
    static const char *const COMMANDS[] = {"status", "type", "key", "move", "moveto", "click", "down", "up", "scroll", "release"};
    bool known = false;
    for (size_t i = 0; argc > 1 && i < sizeof(COMMANDS) / sizeof(COMMANDS[0]); ++i) {
        known = known || strcmp(argv[1], COMMANDS[i]) == 0;
    }
    if (!known) {
        return false;
    }
    static char line[WYE_API_LINE];
    *exit_code = 1;
    if (!build_line(argc, argv, line, sizeof(line))) {
        fprintf(stderr, "wye: command too long\n");
        return true;
    }
    plat_ipc *c = plat_ipc_connect();
    if (!c) {
        fprintf(stderr, "wye: cannot reach Wye. Start it, and turn on the API under Settings.\n");
        *exit_code = 2;
        return true;
    }
    strcat(line, "\n");
    char reply[512];
    size_t got = 0;
    if (plat_ipc_write(c, line, strlen(line))) {
        while (got < sizeof(reply) - 1 && !memchr(reply, '\n', got)) {
            int n = plat_ipc_read(c, reply + got, sizeof(reply) - 1 - got, REPLY_TIMEOUT_MS);
            if (n <= 0) {
                break;
            }
            got += (size_t)n;
        }
    }
    plat_ipc_close(c);
    reply[got] = '\0';
    char *nl = strchr(reply, '\n');
    if (!nl) {
        fprintf(stderr, "wye: no reply from the running app\n");
        return true;
    }
    *nl = '\0';
    if (strncmp(reply, "ok", 2) == 0) {
        *exit_code = 0;
        if (reply[2]) {
            printf("%s\n", reply + 3);
        }
    } else {
        fprintf(stderr, "wye: %s\n", strncmp(reply, "err ", 4) == 0 ? reply + 4 : reply);
    }
    return true;
}
