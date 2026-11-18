/* Copyright (c) 2026 bhgv
 * EN: Refuse every native wait setup allocation, preserving queued request ownership and exact native retirement.
 * RU: Отказать каждому native wait setup allocation, сохраняя queued request ownership и точное native retirement.
 */
#include "../backend.h"
#include "../../../../runtime/vfs/launch/service.h"
static void *live[64]; /* Native wait allocations only, no observer allocation or managed pointers. */
static int tracking; /* Main-thread-only fixture; no other workers/callback publishers are started. */
static unsigned allocation; /* Actual malloc/calloc positions within the public wait call. */
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


/* EN/RU: Record actual successful allocation only during wait setup; return unchanged owned pointer.
 * Учесть настоящую successful allocation лишь при wait setup; вернуть неизменённый owned pointer.
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
        launch_check(0, "wait allocation ledger overflow");
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


/* EN/RU: Observe actual free of wait setup storage even after tracking stops; ownership unchanged.
 * Наблюдать настоящий free wait setup storage даже после tracking; ownership не меняется.
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


/* EN/RU: Execute one borrowed-service wait with selected setup refusal, verify quota unchanged on failure and complete drain.
 * Выполнить wait с selected setup refusal, проверить неизменную quota при failure и полный drain.
 */
static unsigned attempt(DVM_LaunchOptions *options, unsigned failure)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    DVM_LaunchSpec spec = {0}; /* All request allocation happens outside the observed wait. */
    spec.policy = options->policy;
    spec.terminal = options->terminal;
    spec.terminal_origin = options->terminal_origin;
    strcpy(spec.expected_provider, options->provider_origin.system);
    strcpy(spec.module, "allocation-survivor");
    DVM_LaunchTicket *ticket = dvm_launch_service_submit(endpoint, &spec);
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    launch_check(owner && endpoint && ticket && cancel && dvm_cancel_enter(&scope, cancel), "allocation wait setup");
    DVM_LaunchReview *review = NULL;
    allocation = 0;
    fail_at = failure;
    tracking = 1;
    DVM_LaunchWaitResult result = dvm_launch_service_wait(owner, &review);
    tracking = 0;
    unsigned positions = allocation; /* Only actual allocations attempted before this outcome. */
    if (failure)
    {
        DVM_LaunchServiceInfo info;
        launch_check(result == DVM_LAUNCH_WAIT_FAILED && !review &&
            dvm_launch_service_info(owner, &info) && info.outstanding == 1 && info.queued == 1 &&
            dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING,
            "wait setup failure consumed request or quota");
        launch_check(dvm_launch_service_wait(owner, &review) == DVM_LAUNCH_WAIT_REQUEST,
            "setup failure poisoned later wait");
    }
    else
    {
        launch_check(result == DVM_LAUNCH_WAIT_REQUEST && review, "baseline allocation wait");
    }
    launch_check(dvm_cancel_leave(&scope), "allocation wait scope release");
    dvm_cancel_release(cancel);
    dvm_launch_review_free(review);
    dvm_launch_ticket_free(ticket);
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(owner);
    for (unsigned i = 0; i < sizeof(live) / sizeof(*live); i++)
    {
        launch_check(!live[i], "native wait setup allocation leaked");
    }
    return positions;
}


/* EN/RU: Establish actual allocation count, fail every position, require independent source/root cleanup.
 * Определить actual allocation count, отказать каждой position, требовать source/root cleanup.
 */
int main(void)
{
    LaunchState terminal = {0};
    LaunchState provider = {0};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    unsigned count = attempt(&options, 0);
    launch_check(count > 3 && count < 64, "wait allocation coverage unexpectedly incomplete");
    for (unsigned i = 1; i <= count; i++)
    {
        (void)attempt(&options, i);
    }
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "wait allocation root ownership drain");
    printf("PASS all %u native launch wait allocation refusal positions\n", count);
    return 0;
}
