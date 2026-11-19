/* Copyright (c) 2026 bhgv
 * EN: Refuse every filtered provider-discovery allocation, preserving live policy and native ownership.
 * RU: Отказать каждой allocation filtered provider-discovery, сохраняя live policy и native ownership.
 */
#include "source.h"
static void *live[64]; /* Native route allocations only, no observer allocation or managed pointers. */
static int tracking; /* Main-thread-only fixture; no other workers/callback publishers are started. */
static unsigned allocation; /* Actual malloc/calloc positions within the public route call. */
static unsigned fail_at; /* Zero allows all setup allocations; nonzero fails exactly this one. */


/* EN/RU: Original native allocator, returns owned storage/NULL for bytes.
 * Исходный native allocator, возвращает owned storage/NULL для bytes.
 */
extern void *__real_malloc(size_t bytes);


/* EN/RU: Original zeroed allocator, returns owned storage/NULL for count*bytes.
 * Исходный zeroed allocator, возвращает owned storage/NULL для count*bytes.
 */
extern void *__real_calloc(size_t count, size_t bytes);


/* EN/RU: Original free consumes native allocation/NULL without reentering the fixture.
 * Исходный free поглощает native allocation/NULL, не входит в fixture повторно.
 */
extern void __real_free(void *pointer);


/* EN/RU: Record actual successful allocation only during routed submit; return unchanged owned pointer.
 * Учесть настоящую successful allocation лишь при routed submit; вернуть неизменённый owned pointer.
 */
static void *record(void *pointer)
{
    if (pointer && tracking)
    {
        for (unsigned i = 0; i < sizeof(live) / sizeof(*live); i++)
        {
            if (!live[i])
            {
                live[i] = pointer;
                return pointer;
            }
        }
        launch_check(0, "route allocation ledger overflow");
    }
    return pointer;
}


/* EN/RU: Refuse exact malloc position, otherwise preserve real allocator result and track its ownership.
 * Отказать точной malloc position, иначе сохранить real allocator result и учесть ownership.
 */
void *__wrap_malloc(size_t size)
{
    if (tracking && ++allocation == fail_at)
    {
        return NULL;
    }
    return record(__real_malloc(size));
}


/* EN/RU: Refuse exact calloc position, otherwise preserve real allocator result and track its ownership.
 * Отказать точной calloc position, иначе сохранить real allocator result и учесть ownership.
 */
void *__wrap_calloc(size_t count, size_t size)
{
    if (tracking && ++allocation == fail_at)
    {
        return NULL;
    }
    return record(__real_calloc(count, size));
}


/* EN/RU: Observe actual free of route storage even after tracking stops; ownership unchanged.
 * Наблюдать настоящий free route storage даже после tracking; ownership не меняется.
 */
void __wrap_free(void *pointer)
{
    for (unsigned i = 0; i < sizeof(live) / sizeof(*live); i++)
    {
        if (pointer && live[i] == pointer)
        {
            live[i] = NULL;
            break;
        }
    }
    __real_free(pointer);
}


/* EN/RU: Run one real view-discovery submit with selected allocation failure; guard/source/context owners must all retire.
 * Выполнить real view-discovery submit с selected allocation failure; guard/source/context owners завершаются полностью.
 */
static unsigned attempt(DVM_LaunchOptions *options, unsigned failure)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    SourceStats stats = {0};
    DVM_Provider *source = source_create(endpoint, &stats, ".", SOURCE_FLAGS, 1);
    Policy policy = {.allowed = 1};
    DVM_View *builder = dvm_view_create(policy_check, &policy, policy_release);
    launch_check(owner && endpoint && source && builder &&
        dvm_view_map(builder, "/run", source, "dev/keyboard", FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0),
        "discovery allocation view builder");
    DVM_Provider *view = dvm_view_freeze(builder);
    DVM_Namespace *group = dvm_launch_prepare(options);
    launch_check(view && group && dvm_stack_mount(group, view, "/", 0, 0) == 1, "discovery allocation namespace");
    DVM_LaunchSpec request = {0};
    request.policy = options->policy;
    request.terminal = options->terminal;
    request.terminal_origin = options->terminal_origin;
    memset(request.module, 'x', sizeof(request.module));
    memset(request.expected_provider, 'x', sizeof(request.expected_provider));
    allocation = 0;
    fail_at = failure;
    tracking = 1;
    DVM_LaunchTicket *ticket = dvm_launch_route_discover(group, "/run", DVM_ORIGIN_PROVIDER, &request);
    tracking = 0;
    unsigned positions = allocation;
    if (failure)
    {
        DVM_LaunchServiceInfo info;
        launch_check(!ticket && dvm_launch_service_info(owner, &info) && !info.outstanding && !info.queued,
            "discovery failure leaked partial admission");
        ticket = dvm_launch_route_discover(group, "/run", DVM_ORIGIN_PROVIDER, &request);
        launch_check(ticket != NULL, "discovery failure poisoned source/policy");
    }
    source_received(owner, ticket);
    dvm_namespace_release(group);
    dvm_provider_release(view);
    dvm_provider_release(source);
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(owner);
    launch_check(stats.created == stats.destroyed && policy.released == 1, "discovery source/policy ownership leaked");
    for (unsigned i = 0; i < sizeof(live) / sizeof(*live); i++)
    {
        launch_check(!live[i], "discovery native allocation owner leaked");
    }
    return positions;
}


/* EN/RU: Measure actual allocations, refuse every position and require native root drain; no mocked module result.
 * Измерить actual allocations, отказать каждой position и требовать native root drain; без mocked module result.
 */
int main(void)
{
    LaunchState terminal = {.terminal = 1};
    LaunchState provider = {0};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    unsigned count = attempt(&options, 0);
    launch_check(count > 8 && count < 64, "filtered discovery allocation coverage incomplete");
    for (unsigned i = 1; i <= count; i++)
    {
        (void)attempt(&options, i);
    }
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "filtered discovery caller root drain");
    printf("PASS all %u filtered discovery allocation refusal positions\n", count);
    return 0;
}
