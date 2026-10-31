/* Helpers over libdbus's iterators. Appends only fail when memory runs out, so callers chain them. */
#include "dbus_lib.h"

bool dbus_arg(DBusMessage *m, int index, int type, void *out)
{
    dbus_iter it;
    if (!dbus.iter_init(m, &it)) {
        return false;
    }
    for (int i = 0; i < index; i++) {
        if (!dbus.iter_next(&it)) {
            return false;
        }
    }
    if (dbus.arg_type(&it) != type) {
        return false;
    }
    dbus.get_basic(&it, out);
    return true;
}

bool dbus_put_str(dbus_iter *it, const char *s)
{
    return dbus.append_basic(it, 's', &s);
}

bool dbus_put_path(dbus_iter *it, const char *p)
{
    return dbus.append_basic(it, 'o', &p);
}

bool dbus_put_i32(dbus_iter *it, int32_t v)
{
    return dbus.append_basic(it, 'i', &v);
}

bool dbus_put_u32(dbus_iter *it, uint32_t v)
{
    return dbus.append_basic(it, 'u', &v);
}

bool dbus_put_bool(dbus_iter *it, bool v)
{
    int32_t b = v ? 1 : 0; /* D-Bus booleans are 32 bits */
    return dbus.append_basic(it, 'b', &b);
}

/* One {key: value} entry of an a{sv} dictionary, with a basic value of the given type. */
bool dbus_put_kv(dbus_iter *arr, const char *key, int type, const void *value)
{
    dbus_iter entry, var;
    char sig[2] = {(char)type, 0};
    return dbus.open_container(arr, 'e', NULL, &entry) && dbus_put_str(&entry, key) &&
           dbus.open_container(&entry, 'v', sig, &var) && dbus.append_basic(&var, type, value) &&
           dbus.close_container(&entry, &var) && dbus.close_container(arr, &entry);
}

/* A variant holding whatever write() appends for name. */
bool dbus_put_variant(dbus_iter *parent, const char *sig, dbus_value_fn write, const char *name)
{
    dbus_iter var;
    return dbus.open_container(parent, 'v', sig, &var) && write(&var, name) && dbus.close_container(parent, &var);
}

/* One {key: variant} entry of an a{sv} dictionary, the variant filled in by write(). */
bool dbus_put_entry(dbus_iter *arr, const char *key, const char *sig, dbus_value_fn write, const char *name)
{
    dbus_iter entry;
    return dbus.open_container(arr, 'e', NULL, &entry) && dbus_put_str(&entry, key) &&
           dbus_put_variant(&entry, sig, write, name) && dbus.close_container(arr, &entry);
}

bool dbus_send_unref(DBusConnection *c, DBusMessage *m)
{
    bool ok = c && m && dbus.send(c, m, NULL);
    if (ok) {
        dbus.flush(c);
    }
    if (m) {
        dbus.unref(m);
    }
    return ok;
}
