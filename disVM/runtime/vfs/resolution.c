/* Copyright (c) 2026 bhgv
 * EN: Coherent virtual binding selection with retained directory resources.
 * RU: Согласованный virtual binding selection с удержанием directory resources.
 */
#include "internal.h"
#include <string.h>


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
 * RU: Разрешить под одним mutex; опционально удержать anchor/relative. Outputs
 * принадлежат caller, group удерживается. Без I/O. Covering rebind не меняет cwd;
 * descendant bindings идут по virtual components. '..' вне cwd — virtual root lookup. */
static int resolve(
    DVM_Namespace *group, const char *path, int module, DVM_NamespacePlan *names, DVM_Anchor **retained, char *relative)
{
    DVM_NamespaceBinding *binding, *best = NULL;
    DVM_Anchor *anchor;
    const char *context, *source, *suffix, *component = "";
    size_t longest = 0;
    int sticky, result = 0;
    fvm_port_sync_lock(group->lock);
    context = module ? group->load_directory : group->cwd;
    anchor = module ? group->load_anchor : group->cwd_anchor;
    if (dvm_namespace_normalize(context, path, names->path))
    {
        sticky = anchor && *path != '/' && covers(context, names->path);
        for (binding = group->bindings; binding; binding = binding->next)
        {
            size_t length = strlen(binding->on);
            if (sticky && length <= strlen(context))
            {
                continue;
            }
            if (length > longest && covers(binding->on, names->path))
            {
                longest = length;
                best = binding;
            }
        }
        if (best)
        {
            anchor = best->anchor;
            source = best->source;
            suffix = names->path + (longest == 1 ? 0 : longest);
            component = best->component;
        }
        else if (sticky)
        {
            source = module ? group->load_source : group->cwd_source;
            suffix = names->path + (strlen(context) == 1 ? 0 : strlen(context));
        }
        else
        {
            anchor = group->root_anchor;
            source = group->root;
            suffix = names->path;
        }
        result = display_join(source, suffix, names->native);
        names->version = group->version;
        if (result && retained)
        {
            result = relative_join(component, suffix, relative) && dvm_anchor_retain(anchor);
            if (result)
            {
                *retained = anchor;
            }
        }
    }
    fvm_port_sync_unlock(group->lock);
    return result;
}


/* EN/RU: See namespace.h; no resource reference escapes a names-only plan.
 * См. namespace.h; names-only plan не передаёт resource reference. */
int dvm_namespace_plan(DVM_Namespace *group, const char *path, DVM_NamespacePlan *plan)
{
    return resolve(group, path, 0, plan, NULL, NULL);
}


/* EN/RU: See namespace.h; coherent import-directory selection.
 * См. namespace.h; согласованный выбор import directory. */
int dvm_namespace_module_plan(DVM_Namespace *group, const char *path, DVM_NamespacePlan *plan)
{
    return resolve(group, path, 1, plan, NULL, NULL);
}


/* EN/RU: See resource.h; output must not already own a live plan.
 * См. resource.h; output не должен уже владеть живым plan. */
int dvm_resource_plan(DVM_Namespace *group, const char *path, int module, DVM_ResourcePlan *plan)
{
    memset(plan, 0, sizeof(*plan));
    return resolve(group, path, module, &plan->names, &plan->anchor, plan->relative) && dvm_resource_register(plan);
}
