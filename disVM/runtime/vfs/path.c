/* Copyright (c) 2026 bhgv
 * EN: Bounded lexical virtual paths and immutable host-path snapshots.
 * RU: Ограниченные лексические virtual paths и независимые снимки host-путей.
 */
#include "internal.h"
#include <string.h>


/* EN/RU: See internal.h; borrowed input/output must not overlap.
 * См. internal.h; заимствованные input/output не должны перекрываться. */
int dvm_namespace_normalize(const char *cwd, const char *path, char *output)
{
    char joined[DVM_NS_PATH_LIMIT];
    size_t used = 1, start, length, position = 0;
    if (!cwd || cwd[0] != '/' || !path || !*path || strlen(path) >= sizeof(joined))
    {
        return 0;
    }
    if (*path == '/')
    {
        memcpy(joined, path, strlen(path) + 1);
    }
    else
    {
        size_t base = strlen(cwd), tail = strlen(path);
        if (base + tail + 2 > sizeof(joined))
        {
            return 0;
        }
        memcpy(joined, cwd, base);
        joined[base] = '/';
        memcpy(joined + base + 1, path, tail + 1);
    }
    output[0] = '/';
    while (joined[position])
    {
        while (joined[position] == '/')
        {
            position++;
        }
        start = position;
        while (joined[position] && joined[position] != '/')
        {
            position++;
        }
        length = position - start;
        if (!length || (length == 1 && joined[start] == '.'))
        {
            continue;
        }
        if (length == 2 && joined[start] == '.' && joined[start + 1] == '.')
        {
            while (used > 1 && output[used - 1] != '/')
            {
                used--;
            }
            if (used > 1)
            {
                used--;
            }
            continue;
        }
        if (used > 1)
        {
            output[used++] = '/';
        }
        memcpy(output + used, joined + start, length);
        used += length;
    }
    output[used] = 0;
    return 1;
}


/* EN/RU: Copy prefix+suffix into output, collapsing one joining slash; one/zero.
 * Скопировать prefix+suffix, убрав один стыковой slash; один/ноль. Без I/O. */
static int join_native(const char *prefix, const char *suffix, char *output)
{
    size_t base = strlen(prefix), tail;
    if (base == 1 && prefix[0] == '/' && suffix[0] == '/')
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


/* EN: Resolve borrowed path/plan under group lock; module selects import base
 * rather than cwd. Return one/zero; no I/O, managed access or pointer escape.
 * RU: Разрешить path/plan под mutex; module выбирает import base вместо cwd.
 * Один/ноль; без I/O, managed access и передачи указателей наружу. */
static int plan_path(DVM_Namespace *group, const char *path, DVM_NamespacePlan *plan, int module)
{
    DVM_NamespaceBinding *binding, *best = NULL;
    size_t longest = 0;
    int result = 0;
    fvm_port_sync_lock(group->lock);
    if (dvm_namespace_normalize(module ? group->load_directory : group->cwd, path, plan->path))
    {
        for (binding = group->bindings; binding; binding = binding->next)
        {
            size_t length = strlen(binding->on);
            if (length > longest && !strncmp(binding->on, plan->path, length) &&
                (length == 1 || !plan->path[length] || plan->path[length] == '/'))
            {
                longest = length;
                best = binding;
            }
        }
        const char *suffix = best ? plan->path + (longest == 1 ? 0 : longest) : plan->path;
        result = join_native(best ? best->source : group->root, suffix, plan->native);
        plan->version = group->version;
    }
    fvm_port_sync_unlock(group->lock);
    return result;
}


/* EN/RU: See namespace.h; ordinary paths follow the group cwd.
 * См. namespace.h; обычные пути следуют cwd группы. */
int dvm_namespace_plan(DVM_Namespace *group, const char *path, DVM_NamespacePlan *plan)
{
    return plan_path(group, path, plan, 0);
}


/* EN/RU: See namespace.h; imports use the same frozen binding selection.
 * См. namespace.h; imports используют тот же frozen binding selection. */
int dvm_namespace_module_plan(DVM_Namespace *group, const char *path, DVM_NamespacePlan *plan)
{
    return plan_path(group, path, plan, 1);
}
