/* Copyright (c) 2026 bhgv
 * EN: Provider access policy selection, dynamic rights and absent callbacks.
 * RU: Provider access selection, динамические права и отсутствующий callback.
 */
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Main-owned mutable policy; callbacks are synchronous on this test thread.
 * Main владеет policy; callbacks синхронны на test thread. */
typedef struct Policy
{
    unsigned file; /* Effective rights, independent from reported mode=0777. */
    unsigned parent; /* Effective write/search rights of the directory. */
    int exists; /* File existence for metadata, not directory existence. */
    unsigned closed; /* Last factory reference release count. */
} Policy;


/* EN/RU: Terminate on a false test condition with its label.
 * Завершить при false test condition с указанной подписью. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "auto access: %s\n", label);
        exit(2);
    }
}


/* EN/RU: Report deliberately permissive mode; only access callback is authority.
 * Borrow state/path/output; missing file returns ERROR without changing output.
 * Вернуть намеренно permissive mode; права задаёт только access callback. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    Policy *policy = state;
    int directory = !strcmp(path, ".") || !strcmp(path, "sub");
    if (!directory && !policy->exists)
    {
        return FVM_ERROR;
    }
    memset(output, 0, sizeof(*output));
    output->mode = 0777;
    output->directory = directory;
    return FVM_OK;
}


/* EN/RU: Selection must never open a handle or perform a provider side effect.
 * Selection не должна открывать handle либо делать provider side effect. */
static FVM_Result open_refused(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)output;
    require(0, "selection opened a resource");
    return FVM_ERROR;
}


/* EN/RU: Check the current policy, including the actual nested parent 'sub'.
 * Borrow inputs, no effects or locks. Проверить текущую policy, включая parent sub. */
static FVM_Result access_check(void *state, const char *path, unsigned access)
{
    Policy *policy = state;
    unsigned permitted = !strcmp(path, ".") || !strcmp(path, "sub") ? policy->parent : policy->file;
    return (access & permitted) == access ? FVM_OK : FVM_ERROR;
}


/* EN/RU: Count final factory release, leaving main-owned storage alive.
 * Посчитать final release, оставив main-owned storage живым. */
static void destroy(void *state)
{
    Policy *policy = state;
    policy->closed++;
}
static const DVM_ProviderOps ops = {info, open_refused, destroy, DVM_PROVIDER_STABLE_BIND, access_check}; /* Immutable test factory. */
static const DVM_ProviderOps unknown_access = {info, open_refused, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* No invented mode authority. */


/* EN/RU: Install a provider source then append/bind it at view; retain no local ref.
 * Установить source, затем bind к view; local ref поглощается до return. */
static void mount(DVM_Namespace *group, const char *name, DVM_Provider *provider, unsigned flags)
{
    DVM_NamespacePlan target;
    DVM_ResourcePlan source, on;
    require(dvm_namespace_plan(group, name, &target) &&
            dvm_namespace_mount_provider(group, &target, provider, name) == 1, "source mount");
    require(dvm_resource_plan(group, name, 0, &source) &&
            dvm_resource_plan(group, "/view", 0, &on), "bind plans");
    require(dvm_namespace_bind_layers(group, &source, &on, flags, 1, "") == 1, "bind layers");
    dvm_resource_close(&source);
    dvm_resource_close(&on);
    dvm_provider_release(provider);
}


/* EN/RU: Select fresh plan by operation and require exact source, or explicit error.
 * No handles are opened. Свежий plan по операции: проверить source либо отказ. */
static void selected(DVM_Namespace *group, unsigned access, int creating, const char *expected)
{
    DVM_ResourcePlan plan;
    FVM_Result status;
    require(dvm_resource_plan(group, "/view/sub/item", 0, &plan), "selection plan");
    plan.requested_access = access;
    status = dvm_union_select(&plan, creating);
    if (expected)
    {
        require(status == FVM_OK && !strcmp(plan.names.native, expected), "wrong permitted source");
    }
    else
    {
        require(status != FVM_OK && !plan.selected, "all denied selection succeeded");
    }
    dvm_resource_close(&plan);
}


/* EN/RU: Exercise different R/W/X branches, live changes, copied policy, parent
 * w without x, missing-file create, and missing callback despite mode=0777.
 * Проверить R/W/X ветви, изменения, copy, parent w без x, missing create и callback. */
int main(void)
{
    Policy a = {FVM_ACCESS_READ, FVM_ACCESS_WRITE, 1, 0};
    Policy b = {FVM_ACCESS_WRITE, FVM_ACCESS_WRITE | FVM_ACCESS_EXEC, 1, 0};
    Policy c = {FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0, 1, 0};
    Policy unknown = {FVM_ACCESS_MASK, FVM_ACCESS_MASK, 1, 0};
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL), *copy;
    require(group != NULL, "namespace");
    mount(group, "/a", dvm_provider_create(&ops, &a), DVM_NS_AUTORWX);
    mount(group, "/b", dvm_provider_create(&ops, &b), DVM_NS_AFTER);
    mount(group, "/c", dvm_provider_create(&ops, &c), DVM_NS_AFTER);
    mount(group, "/unknown", dvm_provider_create(&unknown_access, &unknown), DVM_NS_BEFORE);
    selected(group, FVM_ACCESS_READ, 0, "/a/sub/item");
    selected(group, FVM_ACCESS_WRITE, 0, "/b/sub/item");
    selected(group, FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0, "/c/sub/item");
    selected(group, FVM_ACCESS_WRITE, 1, "/b/sub/item");
    b.exists = 0;
    selected(group, FVM_ACCESS_WRITE, 1, "/b/sub/item");
    copy = dvm_namespace_copy(group);
    require(copy != NULL, "copy");
    a.file = FVM_ACCESS_WRITE;
    selected(copy, FVM_ACCESS_WRITE, 0, "/a/sub/item");
    a.parent |= FVM_ACCESS_EXEC;
    selected(group, FVM_ACCESS_WRITE, 1, "/a/sub/item");
    a.file = b.file = c.file = 0;
    selected(group, FVM_ACCESS_WRITE, 0, NULL);
    selected(group, FVM_ACCESS_MASK << 1, 0, NULL);
    dvm_namespace_release(group);
    dvm_namespace_release(copy);
    require(a.closed == 1 && b.closed == 1 && c.closed == 1 && unknown.closed == 1, "provider lifetime");
    puts("PASS provider MAUTORWX R/W/X/copy/changing-rights/parent-search/absent-callback/no-effects");
    return 0;
}
