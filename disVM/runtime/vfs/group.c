/* Copyright (c) 2026 bhgv
 * EN: Shared/copy namespace ownership independent of guest heap lifetime.
 * RU: Владение общими/копируемыми namespace независимо от guest heap.
 */
#include "internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: See namespace.h. Input roots are absolute host paths, not capabilities.
 * См. namespace.h. Root — абсолютный host path, не capability. */
DVM_Namespace *dvm_namespace_create(const char *root, const char *cwd, const char *load_directory)
{
    DVM_Namespace *group = calloc(1, sizeof(*group));
    if (!group)
    {
        return NULL;
    }
    if (!root || *root != '/' || !cwd || *cwd != '/' || !dvm_namespace_normalize("/", root, group->root) ||
        !dvm_namespace_normalize("/", cwd, group->cwd) ||
        !dvm_namespace_normalize(group->cwd, load_directory ? load_directory : group->cwd, group->load_directory))
    {
        free(group);
        return NULL;
    }
    group->lock = fvm_port_sync_create();
    if (!group->lock)
    {
        free(group);
        return NULL;
    }
    group->references = 1;
    return group;
}


/* EN/RU: See namespace.h. Caller already owns a reference guarding lock lifetime.
 * См. namespace.h. Caller уже держит ссылку, защищающую lifetime mutex. */
int dvm_namespace_retain(DVM_Namespace *group)
{
    int result = 0;
    fvm_port_sync_lock(group->lock);
    if (group->references < (size_t)-1)
    {
        group->references++;
        result = 1;
    }
    fvm_port_sync_unlock(group->lock);
    return result;
}


/* EN/RU: See namespace.h; last reference excludes all plan/commit/copy callers.
 * См. namespace.h; последняя ссылка исключает всех plan/commit/copy callers. */
void dvm_namespace_release(DVM_Namespace *group)
{
    int last;
    if (!group)
    {
        return;
    }
    fvm_port_sync_lock(group->lock);
    last = --group->references == 0;
    fvm_port_sync_unlock(group->lock);
    if (last)
    {
        DVM_NamespaceBinding *binding = group->bindings;
        while (binding)
        {
            DVM_NamespaceBinding *next = binding->next;
            free(binding);
            binding = next;
        }
        fvm_port_sync_destroy(group->lock);
        free(group);
    }
}


/* EN/RU: See namespace.h. Copy only native data; discard partial copy on OOM.
 * См. namespace.h. Копировать native data; при OOM удалить неполную копию. */
DVM_Namespace *dvm_namespace_copy(DVM_Namespace *group)
{
    DVM_Namespace *copy;
    DVM_NamespaceBinding *binding;
    fvm_port_sync_lock(group->lock);
    copy = dvm_namespace_create(group->root, group->cwd, group->load_directory);
    if (copy)
    {
        for (binding = group->bindings; binding; binding = binding->next)
        {
            DVM_NamespaceBinding *entry = malloc(sizeof(*entry));
            if (!entry)
            {
                dvm_namespace_release(copy);
                copy = NULL;
                break;
            }
            memcpy(entry, binding, sizeof(*entry));
            entry->next = copy->bindings;
            copy->bindings = entry;
        }
    }
    fvm_port_sync_unlock(group->lock);
    return copy;
}
