/* Folder-wide operations: counting directory entries, deleting trees. */
#include "fat_tree.h"
#include <stdlib.h>
#if defined(_WIN32)
#include <direct.h>
#define rmdir _rmdir
#else
#include <unistd.h>
#endif

int tree_slots(const char *path, bool sub)
{
    struct tree_names names;
    struct tree_item it;
    int slots = sub ? 2 : 0;
    if (!tree_list(path, &names)) {
        return slots;
    }
    for (size_t i = 0; i < names.count; ++i) {
        char *full = tree_join(path, names.items[i]);
        slots += full && tree_stat(full, &it) ? fat_lfn_slots(names.items[i]) + 1 : 0;
        free(full);
    }
    tree_free(&names);
    return slots;
}

bool tree_remove(const char *path)
{
    struct tree_names names;
    struct tree_item it;
    if (tree_stat(path, &it) && it.is_dir && tree_list(path, &names)) {
        for (size_t i = 0; i < names.count; ++i) {
            char *full = tree_join(path, names.items[i]);
            tree_remove(full);
            free(full);
        }
        tree_free(&names);
        return rmdir(path) == 0;
    }
    return remove(path) == 0;
}

bool tree_rmdir(const char *path)
{
    return rmdir(path) == 0;
}
