/* Copyright (c) 2026 bhgv
 * EN: Refuse every routed-address/submission allocation, preserving admission quotas and exact native ownership.
 * RU: Отказать каждой allocation routed-address/submission, сохраняя admission quotas и native ownership.
 */
#include "test.h"
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


/* EN/RU: Native resolver borrows the exact source, creates a real owner-scoped address/NULL; no provider/guest execution.
 * Native resolver заимствует exact source, создаёт настоящий owner-scoped address/NULL; без provider/guest execution.
 */
static DVM_LaunchAddress *locate(void *context, const DVM_ResourcePlan *source, const FVM_FileInfo *metadata)
{
    DVM_LaunchEndpoint *endpoint = context;
    launch_check(source->selected && source->provider && !metadata->directory, "allocation route selected source");
    return dvm_launch_address_create(endpoint, "allocation-route");
}


/* EN/RU: Execute one actual routed submit with selected native allocation refusal, require no leaked quota or allocations.
 * Выполнить настоящий routed submit с selected allocation refusal, требовать отсутствие quota/allocation leaks.
 */
static unsigned attempt(DVM_LaunchOptions *options, unsigned failure)
{
    DVM_Namespace *group = dvm_launch_prepare(options);
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    launch_check(group && owner && endpoint, "allocation route fixture");
    DVM_LaunchSpec spec = route_input(options);
    const unsigned char bytes[] = {0, 255, 128, 1};
    spec.arguments = bytes;
    spec.argument_bytes = sizeof(bytes);
    allocation = 0;
    fail_at = failure;
    tracking = 1;
    DVM_LaunchTicket *ticket = dvm_launch_route_submit(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &spec, locate, endpoint);
    tracking = 0;
    unsigned positions = allocation;
    if (failure)
    {
        DVM_LaunchServiceInfo info;
        launch_check(!ticket && dvm_launch_service_info(owner, &info) && !info.outstanding && !info.queued,
            "route allocation failure published partial request or leaked quota");
        ticket = dvm_launch_route_submit(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &spec, locate, endpoint);
        launch_check(ticket != NULL, "route allocation failure poisoned later submission");
    }
    received(owner, ticket, "allocation-route", options->provider_origin.system);
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(owner);
    dvm_namespace_release(group);
    for (unsigned i = 0; i < sizeof(live) / sizeof(*live); i++)
    {
        launch_check(!live[i], "route native allocation owner leaked");
    }
    return positions;
}


/* EN/RU: Establish actual allocation count, refuse every position and verify exact root retirement.
 * Определить actual allocation count, отказать каждой position и проверить точное root retirement.
 */
int main(void)
{
    LaunchState terminal = {.terminal = 1};
    LaunchState provider = {0};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    unsigned count = attempt(&options, 0);
    launch_check(count > 6 && count < 64, "route allocation coverage unexpectedly incomplete");
    for (unsigned i = 1; i <= count; i++)
    {
        (void)attempt(&options, i);
    }
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "route allocation root drain");
    printf("PASS all %u native route allocation refusal positions\n", count);
    return 0;
}
