/* Copyright (c) 2026 bhgv
 * EN: Coherent virtual binding selection with retained directory resources.
 * RU: Согласованный virtual binding selection с удержанием directory resources.
 */
#include "internal.h"
#include <string.h>
#include <stdlib.h>


/* EN/RU: Match a complete component prefix; root covers all absolute paths.
 * Сопоставить полный component prefix; root покрывает все absolute paths. */
static int covers(const char *prefix, const char *path)
{
    size_t length = strlen(prefix);
    return !strncmp(prefix, path, length) && (length == 1 || !path[length] || path[length] == '/');
}


/* EN/RU: Join display prefix/suffix into fixed output; one/zero, no allocation.
 * Соединить display prefix/suffix в output; один/ноль, без allocation. */
static int display_join(const char *prefix, const char *suffix, char *output)
{
    size_t base = strlen(prefix), tail;
    if (base == 1 && prefix[0] == '/' && *suffix == '/')
    {
        suffix++;
    }
    tail = strlen(suffix);
    if (base + tail >= DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    memcpy(output, prefix, base);
    memcpy(output + base, suffix, tail + 1);
    return 1;
}


/* EN/RU: Join relative components, using dot for the selected directory itself.
 * Соединить relative components; точка обозначает сам выбранный каталог. */
static int relative_join(const char *prefix, const char *suffix, char *output)
{
    size_t base = strlen(prefix), tail;
    while (*suffix == '/')
    {
        suffix++;
    }
    tail = strlen(suffix);
    if (base + tail + 2 > DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    memcpy(output, prefix, base);
    if (base && tail)
    {
        output[base++] = '/';
    }
    memcpy(output + base, suffix, tail + 1);
    if (!base && !tail)
    {
        memcpy(output, ".", 2);
    }
    return 1;
}


/* EN: Resolve under one group mutex; optionally retain anchor/output relative.
 * names and optional retained/relative are caller storage; group stays owned.
 * No I/O. Current directory survives a covering rebind; descendant bindings
 * still route by virtual components. '..' outside cwd uses virtual root lookup.
 * module selects import base and stable provider-first candidates within the effective union.
 * RU: Разрешить под одним mutex; опционально удержать anchor/relative. Outputs
 * принадлежат caller, group удерживается. Без I/O. Covering rebind не меняет cwd;
 * descendant bindings идут по virtual components. '..' вне cwd — virtual root lookup.
 * module выбирает import base и stable provider-first candidates своего effective union. */
static int resolve(
    DVM_Namespace *group, const char *path, int module, DVM_NamespacePlan *names, DVM_Anchor **retained, char *relative, DVM_Provider **provider, DVM_ResourcePlan *whole)
{
    DVM_NamespaceBinding *binding, *best = NULL;
    DVM_Anchor *anchor;
    DVM_Provider *selected_provider; /* Borrowed under group lock until retained. */
    const char *context, *source, *suffix, *component = "";
    size_t longest = 0;
    int sticky, result = 0;
    fvm_port_sync_lock(group->lock);
    context = module ? group->load_directory : group->cwd;
    anchor = module ? group->load_anchor : group->cwd_anchor;
    selected_provider = module ? group->load_provider : group->cwd_provider;
    if (dvm_namespace_normalize(context, path, names->path))
    {
        sticky = (anchor || selected_provider) && *path != '/' && covers(context, names->path);
        for (binding = group->bindings; binding; binding = binding->next)
        {
            size_t length = strlen(binding->on);
            if (sticky && length <= strlen(context))
            {
                continue;
            }
            if (covers(binding->on, names->path) && (length > longest ||
                (module && length == longest && best && best->origin.role != DVM_ORIGIN_PROVIDER &&
                 binding->origin.role == DVM_ORIGIN_PROVIDER)))
            {
                longest = length;
                best = binding;
            }
        }
        if (best)
        {
            anchor = best->anchor;
            selected_provider = best->provider;
            source = best->source;
            suffix = names->path + (longest == 1 ? 0 : longest);
            component = best->component;
        }
        else if (sticky)
        {
            component = module ? group->load_component : group->cwd_component;
            source = module ? group->load_source : group->cwd_source;
            suffix = names->path + (strlen(context) == 1 ? 0 : strlen(context));
        }
        else
        {
            anchor = group->root_anchor;
            selected_provider = NULL;
            source = group->root;
            suffix = names->path;
        }
        result = display_join(source, suffix, names->native);
        names->version = group->version;
        if (result && retained)
        {
            result = relative_join(!strcmp(component, ".") ? "" : component, suffix, relative);
            if (result && selected_provider)
            {
                result = provider && dvm_provider_retain(selected_provider);
                if (result)
                {
                    *provider = selected_provider;
                }
            }
            else if (result)
            {
                result = dvm_anchor_retain(anchor);
            }
            if (result)
            {
                *retained = anchor;
            }
        }
    }
    if (result && whole && best)
    {
        DVM_ResourcePlan **tail = &whole->layers;
        whole->origin = best->origin;
        whole->head_id = best->head_identity;
        whole->mount_flags = best->flags;
        whole->policy = best->flags & DVM_NS_AUTORWX;
        whole->layer_id = best->identity;
        for (unsigned pass = 0; pass < (module ? 2U : 1U) && result; pass++)
        {
            for (binding = group->bindings; binding; binding = binding->next)
            {
                DVM_ResourcePlan *choice;
                if (binding == best || strcmp(binding->on, best->on) ||
                    (module && (binding->origin.role == DVM_ORIGIN_PROVIDER) != (pass == 0)))
                {
                    continue;
                }
                choice = calloc(1, sizeof(*choice));
                if (!choice)
                {
                    result = 0;
                    break;
                }
                choice->no_native_fallback = group->no_native_fallback;
                choice->origin = binding->origin;
                choice->names = *names;
                choice->mount_flags = binding->flags;
                whole->policy |= binding->flags & DVM_NS_AUTORWX;
                choice->layer_id = binding->identity;
                if (!display_join(binding->source, suffix, choice->names.native) ||
                    !relative_join(binding->component, suffix, choice->relative) ||
                    !dvm_anchor_retain(binding->anchor))
                {
                    free(choice);
                    result = 0;
                    break;
                }
                choice->anchor = binding->anchor;
                if (!dvm_provider_retain(binding->provider))
                {
                    dvm_anchor_release(choice->anchor);
                    free(choice);
                    result = 0;
                    break;
                }
                choice->provider = binding->provider;
                *tail = choice;
                tail = &choice->layers;
            }
        }
        whole->mount_root = !strcmp(names->path, best->on);
        whole->union_root = whole->layers && whole->mount_root;
    }
    if (result && whole)
    {
        whole->no_native_fallback = group->no_native_fallback;
        whole->absence_only = group->absence_only;
        if (!best && sticky)
        {
            whole->origin = module ? group->load_origin : group->cwd_origin;
        }
    }
    fvm_port_sync_unlock(group->lock);
    return result;
}


/* EN/RU: See namespace.h; no resource reference escapes a names-only plan.
 * См. namespace.h; names-only plan не передаёт resource reference. */
int dvm_namespace_plan(DVM_Namespace *group, const char *path, DVM_NamespacePlan *plan)
{
    return resolve(group, path, 0, plan, NULL, NULL, NULL, NULL);
}


/* EN/RU: See namespace.h; coherent import-directory selection.
 * См. namespace.h; согласованный выбор import directory. */
int dvm_namespace_module_plan(DVM_Namespace *group, const char *path, DVM_NamespacePlan *plan)
{
    return resolve(group, path, 1, plan, NULL, NULL, NULL, NULL);
}


/* EN/RU: See resource.h; output must not already own a live plan.
 * См. resource.h; output не должен уже владеть живым plan. */
int dvm_resource_plan(DVM_Namespace *group, const char *path, int module, DVM_ResourcePlan *plan)
{
    memset(plan, 0, sizeof(*plan));
    plan->group = group;
    if (!resolve(group, path, module, &plan->names, &plan->anchor, plan->relative, &plan->provider, plan))
    {
        dvm_resource_close(plan);
        return 0;
    }
    return dvm_resource_register(plan);
}
