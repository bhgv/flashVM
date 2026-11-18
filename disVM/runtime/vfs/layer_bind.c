/* Copyright (c) 2026 bhgv
 * EN: Ordered directory union publication and exact layer removal.
 * RU: Публикация ordered directory union и удаление exact layer.
 */
#include "internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Consume detached binding chain outside namespace lock.
 * Поглотить detached binding chain вне namespace lock. */
static void release(DVM_NamespaceBinding *entry)
{
    while (entry)
    {
        DVM_NamespaceBinding *next = entry->next;
        dvm_anchor_release(entry->anchor);
        dvm_provider_release(entry->provider);
        free(entry);
        entry = next;
    }
}


/* EN/RU: Capture one validated retained source into an independent binding.
 * Root directory choices own their directory anchor, file source owns parent.
 * Захватить validated source в binding; root dirs удерживают каталог, files parent.
 * Native capture I/O outside group lock; NULL on failure. */
static DVM_NamespaceBinding *capture(DVM_ResourcePlan *source, const char *target,
                                     unsigned flags, int directory, const char *component)
{
    DVM_NamespaceBinding *entry = calloc(1, sizeof(*entry));
    DVM_Anchor *anchor = source->created ? source->created : source->anchor;
    const char *part = source->provider ? source->relative : source->created ? (directory ? "" : component) : source->relative;
    if (!strcmp(part, "."))
    {
        part = "";
    }
    if (!entry || !dvm_anchor_retain(anchor))
    {
        free(entry);
        return NULL;
    }
    entry->anchor = anchor;
    if (source->provider && !dvm_provider_bindable_at(source->provider, source->relative))
    {
        entry->provider = dvm_provider_capture(source->provider, source->relative);
        if (!entry->provider)
        {
            release(entry);
            return NULL;
        }
        part = "";
    }
    else
    {
        if (!dvm_provider_retain(source->provider))
        {
            release(entry);
            return NULL;
        }
        entry->provider = source->provider;
    }
    entry->origin = source->origin; /* A bind changes the visible name, not the selected source association. */
    entry->flags = flags;
    strcpy(entry->on, target);
    strcpy(entry->source, source->names.native);
    strcpy(entry->component, part);
    return entry;
}


/* EN/RU: See resource.h. Allocate/retain all branches before the revision commit.
 * См. resource.h. Выделить/удержать все ветви до revision commit. */
int dvm_namespace_bind_layers(DVM_Namespace *group, DVM_ResourcePlan *source,
                              DVM_ResourcePlan *target, unsigned flags, int directory, const char *component)
{
    DVM_NamespaceBinding *head = NULL, **tail = &head, *baseline = NULL, *old = NULL, **link, *entry;
    DVM_ResourcePlan *choice;
    size_t count = 0;
    uint64_t head_identity; /* Existing mount-head stays live through replace. */
    unsigned order = flags & DVM_NS_ORDER;
    int result = 0;
    if (!group || !source || !target || !component ||
        (flags & ~(DVM_NS_ORDER | DVM_NS_CREATE | DVM_NS_AUTORWX)) || order == DVM_NS_ORDER ||
        (order && !directory) || ((flags & DVM_NS_CREATE) && source->mount_root &&
         (source->layers || !(source->mount_flags & DVM_NS_CREATE))))
    {
        return -1;
    }
    for (choice = source; choice; choice = source->union_root ? choice->layers : NULL)
    {
        if (choice->provider && !dvm_provider_bindable_at(choice->provider, choice->relative) &&
            !dvm_provider_capturable(choice->provider))
        {
            release(head);
            return -1;
        }
        *tail = capture(choice, target->names.path, flags, directory, component);
        if (!*tail)
        {
            release(head);
            return -1;
        }
        count++;
        tail = &(*tail)->next;
    }
    if (order)
    {
        baseline = capture(target, target->names.path, 0, 1, "");
        if (!baseline)
        {
            release(head);
            return -1;
        }
    }
    fvm_port_sync_lock(group->lock);
    if (group->version == target->names.version && group->version == source->names.version)
    {
        result = -1;
        if (group->version != UINT64_MAX && count < UINT64_MAX - group->next_binding)
        {
            for (link = &group->bindings; *link && strcmp((*link)->on, target->names.path); link = &(*link)->next)
            {
            }
            int present = *link != NULL;
            head_identity = present ? (*link)->head_identity : group->next_binding + 1;
            if (!present)
            {
                link = &group->bindings;
            }
            if (!present && order)
            {
                baseline->head_identity = head_identity;
                baseline->identity = ++group->next_binding;
                baseline->next = *link;
                *link = baseline;
                baseline = NULL;
            }
            if (!order)
            {
                while (*link && !strcmp((*link)->on, target->names.path))
                {
                    entry = *link;
                    *link = entry->next;
                    entry->next = old;
                    old = entry;
                }
            }
            else if (order == DVM_NS_AFTER)
            {
                while (*link && !strcmp((*link)->on, target->names.path))
                {
                    link = &(*link)->next;
                }
            }
            for (entry = head; entry; entry = entry->next)
            {
                entry->head_identity = head_identity;
                entry->identity = ++group->next_binding;
            }
            *tail = *link;
            *link = head;
            head = NULL;
            group->version++;
            result = 1;
        }
    }
    fvm_port_sync_unlock(group->lock);
    release(head);
    release(baseline);
    release(old);
    if (result == 1)
    {
        dvm_namespace_changed(group);
    }
    return result;
}


/* EN/RU: See internal.h; remove all on nil filter, exactly one on id/source.
 * См. internal.h; удалить все без filter, ровно один при id/source. */
int dvm_namespace_unmount_exact(DVM_Namespace *group, const DVM_NamespacePlan *target,
                                uint64_t id, const DVM_NamespacePlan *source)
{
    DVM_NamespaceBinding **link, *old = NULL;
    int result = 0;
    fvm_port_sync_lock(group->lock);
    if (group->version == target->version && (!source || source->version == group->version))
    {
        result = -1;
        if (group->version != UINT64_MAX)
        {
            for (link = &group->bindings; *link;)
            {
                DVM_NamespaceBinding *entry = *link;
                if (!strcmp(entry->on, target->path) && (!id || entry->identity == id) &&
                    (!source || !strcmp(entry->source, source->native)))
                {
                    *link = entry->next;
                    entry->next = old;
                    old = entry;
                    result = 1;
                    if (id || source)
                    {
                        break;
                    }
                }
                else
                {
                    link = &entry->next;
                }
            }
            if (result == 1)
            {
                group->version++;
            }
        }
    }
    fvm_port_sync_unlock(group->lock);
    release(old);
    if (result == 1)
    {
        dvm_namespace_changed(group);
    }
    return result;
}


/* EN/RU: See resource.h; id zero removes all target layers.
 * См. resource.h; id ноль удаляет все target layers. */
int dvm_namespace_unmount_layer(DVM_Namespace *group, const DVM_NamespacePlan *target, uint64_t id)
{
    return dvm_namespace_unmount_exact(group, target, id, NULL);
}


/* EN/RU: See internal.h; mount-head identity prevents old FDs seeing a new mount.
 * См. internal.h; head identity не даёт старому FD увидеть новый mount. */
int dvm_namespace_layer_at(DVM_Namespace *group, const char *on, uint64_t head, size_t index,
                            DVM_ResourcePlan *output)
{
    DVM_NamespaceBinding *binding;
    int result = 0;
    fvm_port_sync_lock(group->lock);
    for (binding = group->bindings; binding; binding = binding->next)
    {
        if (binding->head_identity == head && !strcmp(binding->on, on))
        {
            if (index)
            {
                index--;
                continue;
            }
            if (dvm_anchor_retain(binding->anchor))
            {
                output->anchor = binding->anchor;
                if (dvm_provider_retain(binding->provider))
                {
                    output->provider = binding->provider;
                    strcpy(output->names.path, on);
                    strcpy(output->names.native, binding->source);
                    strcpy(output->relative, *binding->component ? binding->component : ".");
                    result = 1;
                }
                else
                {
                    dvm_anchor_release(output->anchor);
                    output->anchor = NULL;
                }
            }
            break;
        }
    }
    fvm_port_sync_unlock(group->lock);
    return result;
}


/* EN/RU: See internal.h; concurrent append cannot extend one read indefinitely.
 * См. internal.h; concurrent append не продлевает один read бесконечно. */
size_t dvm_namespace_layer_count(DVM_Namespace *group, const char *on, uint64_t head)
{
    DVM_NamespaceBinding *binding;
    size_t count = 0;
    fvm_port_sync_lock(group->lock);
    for (binding = group->bindings; binding; binding = binding->next)
    {
        if (binding->head_identity == head && !strcmp(binding->on, on))
        {
            count++;
        }
    }
    fvm_port_sync_unlock(group->lock);
    return count;
}
