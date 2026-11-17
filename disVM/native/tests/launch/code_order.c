/* Copyright (c) 2026 bhgv
 * EN: Provider-first code intents preserve data order, stable classes, denied access and snapshots.
 * RU: Provider-first code intents сохраняют data order, стабильность групп, запреты и snapshots.
 */
#include "backend.h"
#include "../../../runtime/vfs/intent.h"


/* EN/RU: Resolve/read one visible node for the requested purpose; close every acquired owner.
 * Разрешить/прочитать node по purpose; освободить каждый acquired owner, вернуть payload byte. */
static char value(DVM_Namespace *group, const char *path, DVM_ResourceIntent intent)
{
    DVM_ResourcePlan plan;
    FVM_FileInfo info;
    DVM_VfsHandle handle = {0};
    char byte;
    size_t count;
    launch_check(dvm_resource_plan_intent(group, path, intent, &plan) &&
        dvm_resource_info(&plan, &info) == FVM_OK &&
        dvm_provider_open(plan.provider, plan.relative, "r", &handle) == FVM_OK &&
        dvm_vfs_read(&handle, &byte, 1, &count) == FVM_OK && count == 1, "intent node read");
    dvm_vfs_close(&handle);
    dvm_resource_close(&plan);
    return byte;
}


/* EN/RU: Add one owned test branch with explicit trusted provider provenance to an unpublished group.
 * Добавить owned test branch с explicit provider provenance в unpublished group. */
static void branch(DVM_Namespace *group, DVM_Provider *provider, const DVM_Origin *origin)
{
    DVM_NamespaceBinding *binding = calloc(1, sizeof(*binding));
    launch_check(binding && dvm_provider_retain(provider), "third branch retention");
    binding->provider = provider;
    binding->origin = *origin;
    binding->identity = ++group->next_binding;
    binding->head_identity = group->bindings->head_identity;
    strcpy(binding->on, "/");
    strcpy(binding->source, "/");
    DVM_NamespaceBinding *tail = group->bindings;
    while (tail->next)
    {
        tail = tail->next;
    }
    tail->next = binding;
}


/* EN/RU: Exercise every code class without suffix inference and verify data snapshots are untouched.
 * Проверить code classes без suffix inference и сохранность data snapshots. */
int main(void)
{
    LaunchState terminal = {0}, provider = {0}, lower = {0};
    terminal.value = 'T';
    terminal.terminal = 1;
    provider.value = 'P';
    lower.value = 'Q';
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_Namespace *group = dvm_launch_prepare(&options);
    DVM_Provider *third = dvm_provider_create_native(&launch_ops, &lower, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    launch_check(group && third, "code intent fixture");
    branch(group, third, &options.provider_origin);
    DVM_ResourcePlan data;
    launch_check(dvm_resource_plan_intent(group, "/dev/keyboard", DVM_RESOURCE_DATA, &data) &&
        data.origin.role == DVM_ORIGIN_TERMINAL && data.layers->provider == options.provider,
        "data candidate order changed");
    for (int intent = DVM_RESOURCE_DIS; intent <= DVM_RESOURCE_SWF; intent++)
    {
        DVM_ResourcePlan code;
        launch_check(dvm_resource_plan_intent(group, "/dev/keyboard", (DVM_ResourceIntent)intent, &code) &&
            code.provider == options.provider && code.layers->provider == third &&
            code.layers->layers->provider == options.terminal && !code.layers->layers->layers,
            "code groups were not stable provider-first");
        dvm_resource_close(&code);
        launch_check(value(group, "/dev/keyboard", (DVM_ResourceIntent)intent) == 'P' &&
            value(group, "/dev/keyboard", DVM_RESOURCE_DATA) == 'T', "code/data purposes mixed");
    }
    launch_check(data.provider == options.terminal && data.layers->provider == options.provider,
        "code lookup mutated an existing data snapshot");
    dvm_resource_close(&data);
    provider.terminal = 1;
    terminal.terminal = 0;
    launch_check(value(group, "/server", DVM_RESOURCE_DIS) == 'Q', "provider absence skipped lower provider");
    lower.terminal = 1;
    launch_check(value(group, "/server", DVM_RESOURCE_DIS) == 'T', "code absence did not reach terminal");
    unsigned terminal_probes = terminal.denied;
    DVM_ResourcePlan denied;
    FVM_FileInfo info;
    launch_check(dvm_resource_plan_intent(group, "/denied", DVM_RESOURCE_DIS, &denied) &&
        dvm_resource_info(&denied, &info) == FVM_ERROR && terminal.denied == terminal_probes,
        "provider denial became a terminal fallback");
    dvm_resource_close(&denied);
    DVM_Namespace *copy = dvm_namespace_copy(group);
    launch_check(copy && value(copy, "/dev/keyboard", DVM_RESOURCE_SWF) == 'P' &&
        value(copy, "/dev/keyboard", DVM_RESOURCE_DATA) == 'T', "copied namespace lost code order");
    dvm_namespace_release(copy);
    DVM_ResourcePlan invalid;
    launch_check(!dvm_resource_plan_intent(group, "/x", (DVM_ResourceIntent)99, &invalid) &&
        !invalid.provider && !invalid.layers, "unknown resource purpose accepted");
    launch_check(!dvm_resource_plan_intent(group, "", DVM_RESOURCE_DIS, &invalid) &&
        !invalid.group && !invalid.provider && !invalid.layers, "invalid code path left a partial output");
    dvm_namespace_release(group);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    dvm_provider_release(third);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1 && lower.destroyed == 1, "intent plan ownership leak");
    return 0;
}
