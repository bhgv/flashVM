/* Copyright (c) 2026 bhgv
 * EN: Caller-relative typed module selection, stable source classes and live bind/namespace isolation.
 * RU: Caller-relative выбор typed module, стабильные source classes и live bind/namespace isolation.
 */
#include "backend.h"
#include "../../../runtime/vfs/intent.h"
#include <pthread.h>


/* EN/RU: Append one source to a private unpublished namespace, retaining provider; NULL origin stays unclassified.
 * Добавить source в private unpublished namespace с provider pin; NULL origin остаётся unclassified.
 */
static void append(DVM_Namespace *group, DVM_Provider *provider, const DVM_Origin *origin)
{
    DVM_NamespaceBinding *entry = calloc(1, sizeof(*entry));
    launch_check(entry && dvm_provider_retain(provider), "call-order branch allocation");
    entry->provider = provider;
    if (origin)
    {
        entry->origin = *origin;
    }
    strcpy(entry->on, "/");
    strcpy(entry->source, "/");
    entry->identity = ++group->next_binding;
    entry->head_identity = group->bindings->head_identity;
    DVM_NamespaceBinding **tail = &group->bindings;
    while (*tail)
    {
        tail = &(*tail)->next;
    }
    *tail = entry;
}


/* EN/RU: Compare exact retained candidate identities/order, preserving caller-owned frozen plan.
 * Сравнить точные retained candidate identities/order, сохраняя frozen plan caller.
 */
static void order(DVM_ResourcePlan *plan, DVM_Provider **expected, unsigned count)
{
    DVM_ResourcePlan *choice = plan; /* Borrowed chain under plan ownership. */
    for (unsigned i = 0; i < count; i++)
    {
        launch_check(choice && choice->provider == expected[i], "caller-relative candidate class order");
        choice = choice->layers;
    }
    launch_check(!choice, "caller-relative candidate duplication");
}


/* EN/RU: Select a typed module implementation and return its actual source byte; close native owners on return.
 * Выбрать реализацию typed module и вернуть настоящий source byte; освободить native owners до return.
 */
static char value(DVM_Namespace *group, const char *path, unsigned role)
{
    DVM_ResourcePlan plan;
    FVM_FileInfo info;
    DVM_VfsHandle handle = {0};
    size_t count = 0;
    char byte = 0;
    launch_check(dvm_resource_plan_call(group, path, role, &plan) &&
        dvm_resource_info(&plan, &info) == FVM_OK &&
        dvm_provider_open(plan.provider, plan.relative, "r", &handle) == FVM_OK &&
        dvm_vfs_read(&handle, &byte, 1, &count) == FVM_OK && count == 1, "caller-local module source read");
    dvm_vfs_close(&handle);
    dvm_resource_close(&plan);
    return byte;
}


/* EN/RU: Native worker borrows an immutable shared namespace, applying only its own side preference to private plans.
 * Native worker заимствует immutable shared namespace, применяя свой side preference лишь к private plans.
 */
typedef struct Caller
{
    DVM_Namespace *group; /* Borrowed through worker join. */
    unsigned role; /* Trusted caller-relative own-side class for this worker. */
    char expected; /* Actual expected source byte at a shared virtual name. */
} Caller;


/* EN/RU: Repeated selection from each side must not mutate the namespace or the other caller's ordering.
 * Повторный selection с каждой стороны не меняет namespace и ordering другого caller.
 */
static void *calls(void *state)
{
    Caller *caller = state;
    for (unsigned i = 0; i < 256; i++)
    {
        launch_check(value(caller->group, "/dev/keyboard", caller->role) == caller->expected,
            "one caller's side preference changed another caller");
    }
    return NULL;
}


/* EN/RU: Ordinary full-directory and exact-file bind preserve source provenance; explicit child target remains confined.
 * Обычный full-directory и exact-file bind сохраняют source provenance; explicit child target остаётся confined.
 */
static void aliases(DVM_Namespace *group)
{
    DVM_ResourcePlan source;
    DVM_ResourcePlan target;
    launch_check(dvm_resource_plan(group, "/", 0, &source) && dvm_resource_plan(group, "/alias", 0, &target) &&
        dvm_namespace_bind_layers(group, &source, &target, 0, 1, "") == 1, "ordinary union directory bind");
    dvm_resource_close(&target);
    dvm_resource_close(&source);
    launch_check(value(group, "/alias/dev/keyboard", DVM_ORIGIN_TERMINAL) == 'T' &&
        value(group, "/alias/dev/keyboard", DVM_ORIGIN_PROVIDER) == 'P', "directory bind lost own-side source identity");
    launch_check(dvm_resource_plan_call(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &source) &&
        dvm_resource_plan(group, "/single", 0, &target) &&
        dvm_namespace_bind_layers(group, &source, &target, 0, 0, "") == 1, "ordinary exact-file bind");
    dvm_resource_close(&target);
    dvm_resource_close(&source);
    DVM_ResourcePlan single;
    launch_check(dvm_resource_plan_call(group, "/single", DVM_ORIGIN_TERMINAL, &single) &&
        !single.layers && single.origin.role == DVM_ORIGIN_PROVIDER,
        "caller-local preference escaped explicit file binding or lost provenance");
    dvm_resource_close(&single);
    launch_check(value(group, "/single", DVM_ORIGIN_TERMINAL) == 'P', "exact-file bind escaped to ambient local root");
}


/* EN/RU: Only explicit absence permits opposite-side fallback; permission failure never probes a remote alternative.
 * Лишь явное absence допускает opposite-side fallback; permission failure не вызывает remote alternative.
 */
static void fallback(DVM_LaunchOptions *options, LaunchState *terminal, LaunchState *provider)
{
    DVM_Namespace *group = dvm_launch_prepare(options);
    launch_check(group != NULL, "fallback namespace");
    terminal->terminal = 1;
    provider->terminal = 0;
    launch_check(value(group, "/server", DVM_ORIGIN_TERMINAL) == 'P', "client absence failed to reach opposite side");
    terminal->terminal = 0;
    provider->terminal = 1;
    launch_check(value(group, "/server", DVM_ORIGIN_PROVIDER) == 'T', "provider absence failed to reach opposite side");
    for (unsigned role = DVM_ORIGIN_TERMINAL; role <= DVM_ORIGIN_PROVIDER; role++)
    {
        terminal->terminal = role == DVM_ORIGIN_TERMINAL;
        provider->terminal = role == DVM_ORIGIN_PROVIDER;
        unsigned opposite_probes = role == DVM_ORIGIN_TERMINAL ? provider->denied : terminal->denied;
        DVM_ResourcePlan denied;
        FVM_FileInfo info;
        launch_check(dvm_resource_plan_call(group, "/denied", role, &denied) &&
            dvm_resource_info(&denied, &info) == FVM_ERROR &&
            (role == DVM_ORIGIN_TERMINAL ? provider->denied : terminal->denied) == opposite_probes,
            "permission refusal became an opposite-side fallback");
        dvm_resource_close(&denied);
    }
    terminal->terminal = 1;
    provider->terminal = 0;
    dvm_namespace_release(group);
}


/* EN/RU: Exercise stable classes, independent callers, copied namespaces, file-stack movement and retained selections.
 * Проверить stable classes, independent callers, copied namespaces, file-stack movement и retained selections.
 */
int main(void)
{
    LaunchState terminal = {.value = 'T', .terminal = 1};
    LaunchState provider = {.value = 'P'};
    LaunchState extra_terminal = {.value = 'U', .terminal = 1};
    LaunchState extra_provider = {.value = 'Q'};
    LaunchState other = {.value = 'Z'};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_Provider *upper = dvm_provider_create_native(&launch_ops, &extra_terminal, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    DVM_Provider *lower = dvm_provider_create_native(&launch_ops, &extra_provider, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    DVM_Provider *unknown = dvm_provider_create_native(&launch_ops, &other, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    DVM_Namespace *group = dvm_launch_prepare(&options);
    launch_check(group && upper && lower && unknown, "caller-side source fixture");
    append(group, lower, &options.provider_origin);
    append(group, upper, &options.terminal_origin);
    append(group, unknown, NULL);
    DVM_Provider *client_order[] = {options.terminal, upper, options.provider, lower, unknown};
    DVM_Provider *provider_order[] = {options.provider, lower, options.terminal, upper, unknown};
    DVM_Provider *data_order[] = {options.terminal, options.provider, lower, upper, unknown};
    DVM_ResourcePlan client;
    DVM_ResourcePlan server;
    DVM_ResourcePlan data;
    launch_check(dvm_resource_plan_call(group, "/dev/keyboard", DVM_ORIGIN_TERMINAL, &client) &&
        dvm_resource_plan_call(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &server) &&
        dvm_resource_plan(group, "/dev/keyboard", 0, &data), "independent caller plans");
    order(&client, client_order, 5);
    order(&server, provider_order, 5);
    order(&data, data_order, 5);
    Caller callers[2] = {{group, DVM_ORIGIN_TERMINAL, 'T'}, {group, DVM_ORIGIN_PROVIDER, 'P'}};
    pthread_t workers[2];
    uint64_t version = group->version; /* Immutable while both workers hold their own plan storage. */
    for (unsigned i = 0; i < 2; i++)
    {
        launch_check(!pthread_create(&workers[i], NULL, calls, &callers[i]), "caller worker start");
    }
    for (unsigned i = 0; i < 2; i++)
    {
        launch_check(!pthread_join(workers[i], NULL), "caller worker join");
    }
    launch_check(group->version == version, "caller-specific lookup mutated shared namespace");
    order(&client, client_order, 5);
    order(&server, provider_order, 5);
    order(&data, data_order, 5);
    aliases(group);
    DVM_Namespace *copy = dvm_namespace_copy(group);
    launch_check(copy != NULL, "caller namespace copy");
    DVM_ResourcePlan moved;
    launch_check(dvm_resource_plan(group, "/dev/keyboard", 0, &moved) &&
        dvm_stack_move(&moved, moved.names.version, moved.layers->layer_id, -1) == 1,
        "individual file layer move");
    dvm_resource_close(&moved);
    launch_check(value(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER) == 'Q' &&
        value(group, "/dev/keyboard", DVM_ORIGIN_TERMINAL) == 'T' &&
        value(group, "/dev/camera", DVM_ORIGIN_PROVIDER) == 'P' &&
        value(copy, "/dev/keyboard", DVM_ORIGIN_PROVIDER) == 'P' &&
        value(copy, "/dev/keyboard", DVM_ORIGIN_TERMINAL) == 'T',
        "file-only reordering changed sibling/copy or ignored caller side");
    order(&client, client_order, 5);
    order(&server, provider_order, 5);
    order(&data, data_order, 5);
    dvm_resource_close(&client);
    dvm_resource_close(&server);
    dvm_resource_close(&data);
    DVM_ResourcePlan invalid;
    launch_check(!dvm_resource_plan_call(group, "/x", 0, &invalid) && !invalid.group && !invalid.provider && !invalid.layers &&
        !dvm_resource_plan_call(group, "", DVM_ORIGIN_TERMINAL, &invalid) && !invalid.group &&
        !dvm_resource_plan_call(NULL, "/x", DVM_ORIGIN_TERMINAL, &invalid) && !invalid.group,
        "invalid caller-side selection left owners or accepted unknown role");
    fallback(&options, &terminal, &provider);
    dvm_namespace_release(copy);
    dvm_namespace_release(group);
    dvm_provider_release(upper);
    dvm_provider_release(lower);
    dvm_provider_release(unknown);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1 && extra_terminal.destroyed == 1 &&
        extra_provider.destroyed == 1 && other.destroyed == 1, "caller-side provider owner drain");
    puts("PASS client-local and provider-local typed module source selection");
    return 0;
}
