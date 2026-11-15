/* Copyright (c) 2026 bhgv
 * EN: Freeze namespace once before module cache lookup, I/O and parsing.
 * RU: Один frozen namespace plan до module cache lookup, I/O и parsing.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "kernel.h"
#include "isa.h"
#include "runt.h"
#include "module_registry.h"
#include "guest.h"
#include "loader.h"
#include "resource.h"
#include "metadata_wait.h"

extern char *dvm_runtime_file_path(String *); /* Existing rooted guest path validation. */


/* EN/RU: See loader.h. No native allocation survives a guest conversion error.
 * См. loader.h. При ошибке conversion нет native allocation для освобождения. */
char *dvm_namespace_load_name(String *name)
{
    char *path;
    if (name == H)
    {
        return "";
    }
    path = dvm_runtime_file_path(name);
    return path ? path : "";
}


/* [GC:] EN: Consume borrowed path's optional cached reference. Builtins retain
 * their global registry contract; file loads resolve once on ACTIVE source.
 * Cache key and reader receive the same frozen native name, never a second
 * namespace lookup. Return owned Module/NULL, preserving ordinary reader errors.
 * RU: Поглотить optional cached reference для path. Builtins сохраняют global
 * registry; файловый load один раз разрешается ACTIVE source. Cache key/reader
 * получают один frozen native name, без второго namespace lookup. Вернуть
 * владеющий Module/NULL, сохранив обычные reader errors. */
Module *readmod(char *path, Module *cached, int sync)
{
    DVM_ResourcePlan plan;
    DVM_Namespace *group;
    if (path && path[0] == '$')
    {
        return dvm_namespace_native_readmod(path, cached, sync, NULL);
    }
    if (cached)
    {
        unload(cached);
    }
    group = dvm_namespace_current_group();
    if (!group || !dvm_resource_plan(group, path, 1, &plan))
    {
        kwerrstr("invalid or oversized namespace module path");
        return NULL;
    }
    FVM_FileInfo info;
    plan.requested_access = FVM_ACCESS_READ | FVM_ACCESS_EXEC;
    if ((plan.layers || (plan.policy & DVM_NS_AUTORWX)) && dvm_metadata_selected(&plan, &info) != FVM_OK)
    {
        dvm_resource_close(&plan);
        return NULL;
    }
    if (plan.provider)
    {
        dvm_resource_close(&plan);
        kwerrstr("module load from provider is unsupported");
        return NULL;
    }
    cached = dvm_module_registry_lookup(plan.names.native);
    Module *answer = dvm_namespace_native_readmod(plan.names.native, cached, sync, &plan);
    dvm_resource_close(&plan);
    return answer;
}


/* EN/RU: See loader.h; do not accept narrowed identities as cache provenance.
 * См. loader.h; narrowed identities не принимаются как cache provenance. */
int dvm_namespace_module_cacheable(const Dir *metadata)
{
    return metadata && !(metadata->mode & DVM_MODULE_NO_CACHE);
}
