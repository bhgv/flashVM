/* Copyright (c) 2026 bhgv
 * EN: Version-checked namespace updates; validation I/O happens outside locks.
 * RU: Изменения namespace со сверкой версии; проверочный I/O вне mutex.
 */
#include "internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: See namespace.h. Revisions never wrap, so stale plans cannot reappear.
 * См. namespace.h. Версии не переполняются; устаревшие планы не оживают. */
int dvm_namespace_chdir(DVM_Namespace *group, const DVM_NamespacePlan *plan)
{
    int result = 0;
    fvm_port_sync_lock(group->lock);
    if (group->version == plan->version)
    {
        result = -1;
        if (group->version != UINT64_MAX)
        {
            memcpy(group->cwd, plan->path, strlen(plan->path) + 1);
            memcpy(group->load_directory, plan->path, strlen(plan->path) + 1);
            group->version++;
            result = 1;
        }
    }
    fvm_port_sync_unlock(group->lock);
    return result;
}


/* EN/RU: See namespace.h; allocate before locking and free displaced record after.
 * См. namespace.h; выделить до mutex, освободить заменённую запись после mutex. */
int dvm_namespace_bind(DVM_Namespace *group, const DVM_NamespacePlan *source, const DVM_NamespacePlan *target)
{
    DVM_NamespaceBinding *entry = malloc(sizeof(*entry)), *old = NULL, **link;
    int result = 0;
    if (!entry)
    {
        return -1;
    }
    memcpy(entry->on, target->path, strlen(target->path) + 1);
    memcpy(entry->source, source->native, strlen(source->native) + 1);
    fvm_port_sync_lock(group->lock);
    if (group->version == target->version && group->version == source->version)
    {
        result = -1;
        if (group->version != UINT64_MAX)
        {
            for (link = &group->bindings; *link; link = &(*link)->next)
            {
                if (!strcmp((*link)->on, target->path))
                {
                    old = *link;
                    *link = old->next;
                    break;
                }
            }
            entry->next = group->bindings;
            group->bindings = entry;
            entry = NULL;
            group->version++;
            result = 1;
        }
    }
    fvm_port_sync_unlock(group->lock);
    free(entry);
    free(old);
    return result;
}


/* EN/RU: See namespace.h; source comparison uses a frozen host spelling.
 * См. namespace.h; source сравнивается как сохранённый host path. */
int dvm_namespace_unmount(DVM_Namespace *group, const DVM_NamespacePlan *target, const DVM_NamespacePlan *source)
{
    DVM_NamespaceBinding **link, *old = NULL;
    int result = 0;
    fvm_port_sync_lock(group->lock);
    if (group->version == target->version && (!source || source->version == group->version))
    {
        result = -1;
        if (group->version != UINT64_MAX)
        {
            for (link = &group->bindings; *link; link = &(*link)->next)
            {
                if (!strcmp((*link)->on, target->path) && (!source || !strcmp((*link)->source, source->native)))
                {
                    old = *link;
                    *link = old->next;
                    group->version++;
                    result = 1;
                    break;
                }
            }
        }
    }
    fvm_port_sync_unlock(group->lock);
    free(old);
    return result;
}
