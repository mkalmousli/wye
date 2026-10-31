#include "dbus_lib.h"

#include <dlfcn.h>
#include <string.h>

struct dbus_api dbus;

#define LOAD(field, sym)                    \
    do {                                    \
        void *p = dlsym(h, sym);            \
        if (!p) {                           \
            return false;                   \
        }                                   \
        memcpy(&dbus.field, &p, sizeof(p)); \
    } while (0)

static bool load_all(void *h)
{
    LOAD(bus_get, "dbus_bus_get");
    LOAD(bus_request_name, "dbus_bus_request_name");
    LOAD(bus_release_name, "dbus_bus_release_name");
    LOAD(bus_add_match, "dbus_bus_add_match");
    LOAD(add_filter, "dbus_connection_add_filter");
    LOAD(read_write_dispatch, "dbus_connection_read_write_dispatch");
    LOAD(send, "dbus_connection_send");
    LOAD(flush, "dbus_connection_flush");
    LOAD(send_block, "dbus_connection_send_with_reply_and_block");
    LOAD(call_new, "dbus_message_new_method_call");
    LOAD(return_new, "dbus_message_new_method_return");
    LOAD(signal_new, "dbus_message_new_signal");
    LOAD(unref, "dbus_message_unref");
    LOAD(msg_type, "dbus_message_get_type");
    LOAD(member, "dbus_message_get_member");
    LOAD(path, "dbus_message_get_path");
    LOAD(iface, "dbus_message_get_interface");
    LOAD(iter_init, "dbus_message_iter_init");
    LOAD(iter_init_append, "dbus_message_iter_init_append");
    LOAD(append_basic, "dbus_message_iter_append_basic");
    LOAD(open_container, "dbus_message_iter_open_container");
    LOAD(close_container, "dbus_message_iter_close_container");
    LOAD(arg_type, "dbus_message_iter_get_arg_type");
    LOAD(get_basic, "dbus_message_iter_get_basic");
    LOAD(iter_next, "dbus_message_iter_next");
    LOAD(err_init, "dbus_error_init");
    LOAD(err_free, "dbus_error_free");
    LOAD(err_set, "dbus_error_is_set");
    return true;
}

bool dbus_load(void)
{
    static int state; /* 0: not tried yet, 1: loaded, 2: not available */
    if (!state) {
        void *h = dlopen("libdbus-1.so.3", RTLD_NOW | RTLD_LOCAL);
        state = (h && load_all(h)) ? 1 : 2;
    }
    return state == 1;
}
