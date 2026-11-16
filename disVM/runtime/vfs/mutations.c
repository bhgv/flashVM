/* Copyright (c) 2026 bhgv
 * EN: Version-checked namespace updates; validation I/O happens outside locks.
 * RU: Изменения namespace со сверкой версии; проверочный I/O вне mutex.
 */
#include "internal.h"
#include "cancel.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Borrow group/validated plan and mutually exclusive anchor/provider plus bounded component.
 * Retain separate cwd/import owners before locking. Return1 commit,0 stale,-1 refusal;
 * cancellation/revision/identity exhaustion prevents commit; displaced owners retire unlocked.
 * Заимствовать group/plan и anchor/provider с bounded component. Удержать cwd/import до lock;
 * вернуть1/0/-1, отмена/исчерпание не меняет состояние, старые owners освободить вне lock. */
static int change_directory(DVM_Namespace *group, const DVM_NamespacePlan *plan, DVM_Anchor *anchor,
                            DVM_Provider *provider, const char *component)
{
    int result = 0;
    DVM_Anchor *old_cwd = NULL, *old_load = NULL;
    DVM_Provider *old_cwd_provider = NULL, *old_load_provider = NULL;
    if (!dvm_anchor_retain(anchor))
    {
        return -1;
    }
    if (!dvm_anchor_retain(anchor))
    {
        dvm_anchor_release(anchor);
        return -1;
    }
    if (!dvm_provider_retain(provider))
    {
        dvm_anchor_release(anchor);
        dvm_anchor_release(anchor);
        return -1;
    }
    if (!dvm_provider_retain(provider))
    {
        dvm_provider_release(provider);
        dvm_anchor_release(anchor);
        dvm_anchor_release(anchor);
        return -1;
    }
    fvm_port_sync_lock(group->lock);
    if (group->version == plan->version)
    {
        result = -1;
        if (group->version != UINT64_MAX && (!provider || group->next_binding != UINT64_MAX) &&
            !dvm_cancel_requested(dvm_cancel_current()))
        {
            memcpy(group->cwd, plan->path, strlen(plan->path) + 1);
            memcpy(group->load_directory, plan->path, strlen(plan->path) + 1);
            memcpy(group->cwd_source, plan->native, strlen(plan->native) + 1);
            memcpy(group->load_source, plan->native, strlen(plan->native) + 1);
            old_cwd = group->cwd_anchor;
            old_load = group->load_anchor;
            old_cwd_provider = group->cwd_provider;
            old_load_provider = group->load_provider;
            group->cwd_identity = provider ? ++group->next_binding : 0;
            group->cwd_provider = provider;
            group->load_provider = provider;
            strcpy(group->cwd_component, component);
            strcpy(group->load_component, component);
            group->cwd_anchor = anchor;
            group->load_anchor = anchor;
            group->version++;
            result = 1;
        }
    }
    fvm_port_sync_unlock(group->lock);
    if (result == 1)
    {
        dvm_anchor_release(old_cwd);
        dvm_anchor_release(old_load);
        dvm_provider_release(old_cwd_provider);
        dvm_provider_release(old_load_provider);
    }
    else
    {
        dvm_provider_release(provider);
        dvm_provider_release(provider);
        dvm_anchor_release(anchor);
        dvm_anchor_release(anchor);
    }
    if (result == 1)
    {
        dvm_namespace_changed(group);
    }
    return result;
}


/* EN/RU: Pure path commit for native unit clients; see namespace.h.
 * Чистый path commit для native unit clients; см. namespace.h. */
int dvm_namespace_chdir(DVM_Namespace *group, const DVM_NamespacePlan *plan)
{
    return change_directory(group, plan, NULL, NULL, "");
}


/* EN/RU: Commit validated directory identity; see resource.h.
 * Применить проверенную identity каталога; см. resource.h. */
int dvm_namespace_chdir_resource(DVM_Namespace *group, DVM_ResourcePlan *plan)
{
    return plan->created || plan->created_provider ?
        change_directory(group, &plan->names, plan->created, plan->created_provider,
                         plan->created_provider ? plan->created_component : "") : -1;
}


/* EN/RU: See namespace.h; allocate before locking and free displaced record after.
 * См. namespace.h; выделить до mutex, освободить заменённую запись после mutex. */
static int bind_selection(DVM_Namespace *group,
                                const DVM_NamespacePlan *source,
                                const DVM_NamespacePlan *target,
                                DVM_Provider *provider,
                                DVM_Anchor *anchor,
                                const char *component)
{
    DVM_NamespaceBinding *entry = calloc(1, sizeof(*entry)), *old = NULL, **link;
    int result = 0;
    if (!entry)
    {
        return -1;
    }
    if (!component || strlen(component) >= sizeof(entry->component) || !dvm_anchor_retain(anchor))
    {
        free(entry);
        return -1;
    }
    if (!dvm_provider_retain(provider))
    {
        dvm_anchor_release(anchor);
        free(entry);
        return -1;
    }
    entry->provider = provider;
    entry->anchor = anchor;
    memcpy(entry->component, component, strlen(component) + 1);
    memcpy(entry->on, target->path, strlen(target->path) + 1);
    memcpy(entry->source, source->native, strlen(source->native) + 1);
    fvm_port_sync_lock(group->lock);
    if (group->version == target->version && group->version == source->version)
    {
        result = -1;
        if (group->version != UINT64_MAX && group->next_binding != UINT64_MAX)
        {
            entry->identity = ++group->next_binding;
            entry->head_identity = entry->identity;
            for (link = &group->bindings; *link;)
            {
                if (!strcmp((*link)->on, target->path))
                {
                    DVM_NamespaceBinding *removed = *link;
                    *link = removed->next;
                    entry->head_identity = removed->head_identity;
                    removed->next = old;
                    old = removed;
                    if (!*link)
                    {
                        break;
                    }
                    continue;
                }
                link = &(*link)->next;
            }
            entry->next = group->bindings;
            group->bindings = entry;
            entry = NULL;
            group->version++;
            result = 1;
        }
    }
    fvm_port_sync_unlock(group->lock);
    if (entry)
    {
        dvm_provider_release(entry->provider);
        dvm_anchor_release(entry->anchor);
        free(entry);
    }
    while (old)
    {
        DVM_NamespaceBinding *next = old->next;
        dvm_provider_release(old->provider);
        dvm_anchor_release(old->anchor);
        free(old);
        old = next;
    }
    if (result == 1)
    {
        dvm_namespace_changed(group);
    }
    return result;
}


/* EN/RU: Pure path binding; see namespace.h. No native anchor is fabricated.
 * Чистая path binding; см. namespace.h. Native anchor не имитируется. */
int dvm_namespace_bind(DVM_Namespace *group, const DVM_NamespacePlan *source, const DVM_NamespacePlan *target)
{
    return dvm_namespace_bind_resource(group, source, target, NULL, "");
}


/* EN/RU: See namespace.h; source comparison uses a frozen host spelling.
 * См. namespace.h; source сравнивается как сохранённый host path. */
int dvm_namespace_unmount(DVM_Namespace *group, const DVM_NamespacePlan *target, const DVM_NamespacePlan *source)
{
    return dvm_namespace_unmount_exact(group, target, 0, source);
}


/* EN/RU: See resource.h; native bindings keep their existing anchor semantics.
 * См. resource.h; native bindings сохраняют прежнюю семантику anchors. */
int dvm_namespace_bind_resource(DVM_Namespace *group, const DVM_NamespacePlan *source,
                                const DVM_NamespacePlan *target, DVM_Anchor *anchor, const char *component)
{
    return bind_selection(group, source, target, NULL, anchor, component);
}


/* EN/RU: See resource.h; validate label before allocating or acquiring references.
 * См. resource.h; проверить label до allocation или захвата references. */
int dvm_namespace_mount_provider(DVM_Namespace *group, const DVM_NamespacePlan *target,
                                 DVM_Provider *provider, const char *label)
{
    DVM_NamespacePlan source;
    if (!group || !target || !provider || !label || *label != '/' || strlen(label) >= sizeof(source.native))
    {
        return -1;
    }
    source = *target;
    memcpy(source.native, label, strlen(label) + 1);
    return bind_selection(group, &source, target, provider, NULL, "");
}


/* EN/RU: See resource.h; freeze provider/relative source instead of its alias name.
 * См. resource.h; сохранить relative identity source вместо имени alias. */
int dvm_namespace_bind_provider(DVM_Namespace *group, const DVM_ResourcePlan *source,
                                const DVM_NamespacePlan *target)
{
    if (!group || !source || !target)
    {
        return -1;
    }
    if (dvm_provider_bindable_at(source->provider, source->relative))
    {
        return bind_selection(group, &source->names, target, source->provider, NULL,
                              !strcmp(source->relative, ".") ? "" : source->relative);
    }
    DVM_Provider *root = dvm_provider_capture(source->provider, source->relative);
    if (!root)
    {
        return -1;
    }
    int result = bind_selection(group, &source->names, target, root, NULL, "");
    dvm_provider_release(root);
    return result;
}
