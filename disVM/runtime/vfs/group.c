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
    group->notices = dvm_notice_create();
    if (!group->notices)
    {
        fvm_port_sync_destroy(group->lock);
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
            dvm_provider_release(binding->provider);
            dvm_anchor_release(binding->anchor);
            free(binding);
            binding = next;
        }
        if (!dvm_notice_destroy(group->notices))
        {
            abort();
        }
        fvm_port_sync_destroy(group->lock);
        dvm_anchor_release(group->root_anchor);
        dvm_anchor_release(group->cwd_anchor);
        dvm_anchor_release(group->load_anchor);
        dvm_provider_release(group->cwd_provider);
        dvm_provider_release(group->load_provider);
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
        copy->no_native_fallback = group->no_native_fallback;
        copy->absence_only = group->absence_only;
        copy->module_stack_order = group->module_stack_order;
        copy->cwd_origin = group->cwd_origin;
        copy->load_origin = group->load_origin;
        copy->next_binding = group->next_binding;
        copy->cwd_identity = group->cwd_identity;
        memcpy(copy->cwd_source, group->cwd_source, sizeof(copy->cwd_source));
        memcpy(copy->load_source, group->load_source, sizeof(copy->load_source));
        memcpy(copy->cwd_component, group->cwd_component, sizeof(copy->cwd_component));
        memcpy(copy->load_component, group->load_component, sizeof(copy->load_component));
        DVM_Anchor *anchors[] = {group->root_anchor, group->cwd_anchor, group->load_anchor};
        DVM_Anchor **destinations[] = {&copy->root_anchor, &copy->cwd_anchor, &copy->load_anchor};
        size_t index;
        for (index = 0; index < 3; index++)
        {
            if (!dvm_anchor_retain(anchors[index]))
            {
                dvm_namespace_release(copy);
                copy = NULL;
                break;
            }
            *destinations[index] = anchors[index];
        }
    }
    if (copy)
    {
        if (dvm_provider_retain(group->cwd_provider))
        {
            copy->cwd_provider = group->cwd_provider;
            if (dvm_provider_retain(group->load_provider))
            {
                copy->load_provider = group->load_provider;
            }
            else
            {
                dvm_namespace_release(copy);
                copy = NULL;
            }
        }
        else
        {
            dvm_namespace_release(copy);
            copy = NULL;
        }
    }
    if (copy)
    {
        DVM_NamespaceBinding **tail = &copy->bindings;
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
            if (!dvm_anchor_retain(binding->anchor))
            {
                free(entry);
                dvm_namespace_release(copy);
                copy = NULL;
                break;
            }
            if (!dvm_provider_retain(binding->provider))
            {
                dvm_anchor_release(entry->anchor);
                free(entry);
                dvm_namespace_release(copy);
                copy = NULL;
                break;
            }
            entry->next = NULL;
            *tail = entry;
            tail = &entry->next;
        }
    }
    fvm_port_sync_unlock(group->lock);
    return copy;
}


/* EN/RU: See internal.h; mutation publisher retains group through synchronous push.
 * См. internal.h; publisher удерживает group до завершения native push. */
void dvm_namespace_changed(DVM_Namespace *group)
{
    DVM_ChangeEvent event = {0};
    fvm_port_sync_lock(group->lock);
    event.generation = group->version;
    event.kinds = DVM_CHANGE_NAMESPACE;
    fvm_port_sync_unlock(group->lock);
    dvm_notice_publish(group->notices, &event);
}
