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
#include "../module_interface/launch.h"
#include "resource.h"
#include "runqueue.h"
#include "metadata_wait.h"
#include "raise.h"
#include <stdlib.h>

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
 * their global registry contract; file loads resolve once on ACTIVE source,
 * preferring that caller's immutable local source class.
 * Native-host cache and reader receive one frozen name. Provider images use the
 * selected opened-object reader without pathname-cache reuse. sync is forwarded
 * to the unchanged native reader; provider I/O brackets its own source wait.
 * Return owned Module/NULL, preserving ordinary reader errors.
 * RU: Поглотить optional cached reference для path. Builtins сохраняют global
 * registry; файловый load один раз разрешается ACTIVE source. Cache key/reader
 * получают один frozen native name. Provider-образы читаются через выбранный
 * opened-object без cache по имени; sync передаётся native-reader, provider I/O
 * создаёт собственное source-ожидание. Вернуть свой Module/NULL с обычными errors. */
Module *readmod(char *path, Module *cached, int sync)
{
    DVM_ResourcePlan *plan;
    DVM_Namespace *group;
    if (path && path[0] == '$')
    {
        return dvm_launch_module_checked(path, dvm_namespace_native_readmod(path, cached, sync, NULL));
    }
    if (cached)
    {
        unload(cached);
    }
    plan = calloc(1, sizeof(*plan));
    if (!plan)
    {
        kwerrstr("cannot allocate namespace module selection");
        return NULL;
    }
    if (waserror())
    {
        dvm_resource_close(plan);
        free(plan);
        nexterror();
    }
    group = dvm_namespace_current_group();
    Prog *caller = currun(); /* ACTIVE permanent owner only; startup controller has no caller yet. */
    unsigned local_role = caller ? ((DVM_RuntimeProcess *)caller)->module_local_role :
        DVM_ORIGIN_PROVIDER; /* Job-relative own side, not a ban on reverse/third-system calls. */
    if (!group || !dvm_resource_plan_call(group, path, local_role, plan))
    {
        free(plan);
        poperror();
        kwerrstr("invalid or oversized namespace module path");
        return NULL;
    }
    FVM_FileInfo info;
    plan->requested_access = FVM_ACCESS_READ | FVM_ACCESS_EXEC;
    if ((plan->layers || (plan->policy & DVM_NS_AUTORWX)) && dvm_metadata_selected(plan, &info) != FVM_OK)
    {
        dvm_resource_close(plan);
        free(plan);
        poperror();
        return NULL;
    }
    if (plan->provider)
    {
        Module *answer = dvm_provider_readmod(path, plan); /* Same chosen provider, no lower-layer retry. */
        dvm_resource_close(plan);
        free(plan);
        poperror();
        return dvm_launch_module_checked(path, answer);
    }
    cached = dvm_module_registry_lookup(plan->names.native);
    Module *answer = dvm_namespace_native_readmod(plan->names.native, cached, sync, plan);
    dvm_resource_close(plan);
    free(plan);
    poperror();
    return dvm_launch_module_checked(path, answer);
}


/* EN/RU: See loader.h; do not accept narrowed identities as cache provenance.
 * См. loader.h; narrowed identities не принимаются как cache provenance. */
int dvm_namespace_module_cacheable(const Dir *metadata)
{
    return metadata && !(metadata->mode & DVM_MODULE_NO_CACHE);
}
