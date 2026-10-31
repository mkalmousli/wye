/* libdbus, loaded at run time the way PulseAudio is, so building Wye needs no D-Bus headers or libraries. */
#ifndef WYE_DBUS_LIB_H
#define WYE_DBUS_LIB_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct DBusConnection DBusConnection;
typedef struct DBusMessage DBusMessage;
typedef struct { long long opaque[16]; } dbus_iter; /* DBusMessageIter: libdbus fills it in */
typedef struct { long long opaque[8]; } dbus_err;   /* DBusError */
typedef int (*dbus_filter_fn)(DBusConnection *, DBusMessage *, void *);
typedef bool (*dbus_value_fn)(dbus_iter *value, const char *name);

enum {
    WDBUS_SESSION = 0,
    WDBUS_METHOD_CALL = 1,
    WDBUS_SIGNAL = 4,
    WDBUS_HANDLED = 0,
    WDBUS_NOT_HANDLED = 1,
    WDBUS_NAME_DO_NOT_QUEUE = 4,
};

struct dbus_api {
    DBusConnection *(*bus_get)(int type, dbus_err *err);
    int (*bus_request_name)(DBusConnection *c, const char *name, unsigned flags, dbus_err *err);
    int (*bus_release_name)(DBusConnection *c, const char *name, dbus_err *err);
    void (*bus_add_match)(DBusConnection *c, const char *rule, dbus_err *err);
    bool (*add_filter)(DBusConnection *c, dbus_filter_fn fn, void *data, void (*free_data)(void *));
    bool (*read_write_dispatch)(DBusConnection *c, int timeout_ms);
    bool (*send)(DBusConnection *c, DBusMessage *m, uint32_t *serial);
    void (*flush)(DBusConnection *c);
    DBusMessage *(*send_block)(DBusConnection *c, DBusMessage *m, int timeout_ms, dbus_err *err);
    DBusMessage *(*call_new)(const char *dest, const char *path, const char *iface, const char *method);
    DBusMessage *(*return_new)(DBusMessage *call);
    DBusMessage *(*signal_new)(const char *path, const char *iface, const char *name);
    void (*unref)(DBusMessage *m);
    int (*msg_type)(DBusMessage *m);
    const char *(*member)(DBusMessage *m);
    const char *(*path)(DBusMessage *m);
    const char *(*iface)(DBusMessage *m);
    bool (*iter_init)(DBusMessage *m, dbus_iter *it);
    void (*iter_init_append)(DBusMessage *m, dbus_iter *it);
    bool (*append_basic)(dbus_iter *it, int type, const void *value);
    bool (*open_container)(dbus_iter *it, int type, const char *sig, dbus_iter *sub);
    bool (*close_container)(dbus_iter *it, dbus_iter *sub);
    int (*arg_type)(dbus_iter *it);
    void (*get_basic)(dbus_iter *it, void *value);
    bool (*iter_next)(dbus_iter *it);
    void (*err_init)(dbus_err *e);
    void (*err_free)(dbus_err *e);
    bool (*err_set)(const dbus_err *e);
};
extern struct dbus_api dbus;

bool dbus_load(void); /* false when libdbus is not installed */

/* dbus_msg.c: reading arguments, and appending values and variants. */
bool dbus_arg(DBusMessage *m, int index, int type, void *out);
bool dbus_put_str(dbus_iter *it, const char *s);
bool dbus_put_path(dbus_iter *it, const char *p);
bool dbus_put_i32(dbus_iter *it, int32_t v);
bool dbus_put_u32(dbus_iter *it, uint32_t v);
bool dbus_put_bool(dbus_iter *it, bool v);
bool dbus_put_kv(dbus_iter *arr, const char *key, int type, const void *value);
bool dbus_put_variant(dbus_iter *parent, const char *sig, dbus_value_fn write, const char *name);
bool dbus_put_entry(dbus_iter *arr, const char *key, const char *sig, dbus_value_fn write, const char *name);
bool dbus_send_unref(DBusConnection *c, DBusMessage *m);

#endif
