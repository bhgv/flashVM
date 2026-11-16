/* Copyright (c) 2026 bhgv
 * EN: Revision-fenced snapshots of immediate namespace child binding names.
 * RU: Защищённые revision snapshots имён непосредственных child bindings namespace.
 */
#include "directory_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Match immediate children only; parent and path are normalized absolute spellings.
 * Сопоставить только immediate children; parent/path нормализованные absolute spelling. */
static int child(const char *parent, const char *path)
{
    size_t length = strlen(parent);
    if (strncmp(parent, path, length) || (length > 1 && path[length] != '/'))
    {
        return 0;
    }
    const char *name = path+length;
    if (*name == '/')
    {
        name++;
    }
    return *name && !strchr(name, '/');
}


/* EN/RU: See directory_internal.h; allocation outside group mutex, all strings copied inside.
 * См. directory_internal.h; allocation вне group mutex, строки копируются внутри. */
int dvm_directory_names(DVM_Namespace *group, const char *parent, DirectoryName **names, size_t *count)
{
    *names = NULL;
    *count = 0;
    for (unsigned attempt = 0; attempt < 8; attempt++)
    {
        size_t capacity = 0;
        fvm_port_sync_lock(group->lock);
        uint64_t version = group->version;
        for (DVM_NamespaceBinding *entry = group->bindings; entry; entry = entry->next)
        {
            capacity += child(parent, entry->on);
        }
        fvm_port_sync_unlock(group->lock);
        if (!capacity)
        {
            return 1;
        }
        if (capacity > (size_t)-1/sizeof(**names))
        {
            return 0;
        }
        DirectoryName *copied = calloc(capacity, sizeof(*copied));
        if (!copied)
        {
            return 0;
        }
        size_t used = 0;
        fvm_port_sync_lock(group->lock);
        int current = group->version == version;
        if (current)
        {
            for (DVM_NamespaceBinding *entry = group->bindings; entry; entry = entry->next)
            {
                if (!child(parent, entry->on))
                {
                    continue;
                }
                size_t i;
                for (i = 0; i < used && strcmp(copied[i].path, entry->on); i++)
                {
                }
                if (i == used)
                {
                    strcpy(copied[used++].path, entry->on);
                }
            }
        }
        fvm_port_sync_unlock(group->lock);
        if (current)
        {
            *names = copied;
            *count = used;
            return 1;
        }
        free(copied);
    }
    return 0;
}
