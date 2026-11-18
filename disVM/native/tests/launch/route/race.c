/* Copyright (c) 2026 bhgv
 * EN: Exact source pin survives concurrent namespace replacement; queue acknowledgement wins later cancellation.
 * RU: Exact source pin переживает concurrent namespace replacement; queue acknowledgement выигрывает у поздней cancellation.
 */
#include "test.h"
#include "../../../../runtime/vfs/export.h"
#include <pthread.h>
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Test-only resolver/main rendezvous. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Deterministic native barrier without timing sleeps. */
static DVM_Cancel *cancel_after_ack; /* Borrowed only during main-thread synchronous acknowledged-submit test. */


/* EN/RU: Original bounded submit returns an actual acknowledged ticket/NULL, never a fabricated result.
 * Исходный bounded submit выдаёт actual acknowledged ticket/NULL, не fabricated result.
 */
extern DVM_LaunchTicket *__real_dvm_launch_service_submit(DVM_LaunchEndpoint *, const DVM_LaunchSpec *);


/* EN/RU: Request cancellation only after real queue publication/acknowledgement, preserving the real requester capability.
 * Запросить cancellation лишь после настоящей queue publication/acknowledgement, сохранив requester capability.
 */
DVM_LaunchTicket *__wrap_dvm_launch_service_submit(DVM_LaunchEndpoint *endpoint, const DVM_LaunchSpec *spec)
{
    DVM_LaunchTicket *ticket = __real_dvm_launch_service_submit(endpoint, spec);
    if (ticket && cancel_after_ack)
    {
        dvm_cancel_request(cancel_after_ack);
    }
    return ticket;
}


/* EN/RU: Caller-owned native case storage stays alive through all resolver/submit users and their join.
 * Native case storage caller живёт до завершения resolver/submit users и join.
 */
typedef struct RouteCase
{
    DVM_Namespace *group; /* Borrowed source namespace through join. */
    DVM_LaunchSpec *spec; /* Borrowed immutable startup grants/bytes through submit. */
    Routes *routes; /* Borrowed source/endpoint associations. */
    DVM_Provider *expected; /* Actual retained source expected before the test gate. */
    DVM_LaunchTicket *ticket; /* Actual requester result after join/return. */
    int hold; /* Immutable before call: enable resolver gate. */
    int entered; /* Gate-locked: resolver owns the old selected source. */
    int proceed; /* Gate-locked: main finished replacing the live namespace. */
} RouteCase;


/* EN/RU: Hold one selected source while main replaces namespace; route by retained capability, never re-resolve its display name.
 * Удержать selected source при смене namespace; route по retained capability, без повторного lookup display name.
 */
static DVM_LaunchAddress *locate(void *context, const DVM_ResourcePlan *source, const FVM_FileInfo *metadata)
{
    RouteCase *test = context;
    launch_check(source->selected && source->provider == test->expected && !metadata->directory &&
        !strcmp(source->relative, "dev/keyboard"), "route source changed before resolver gate");
    if (test->hold)
    {
        pthread_mutex_lock(&gate);
        test->entered = 1;
        pthread_cond_broadcast(&changed);
        while (!test->proceed)
        {
            pthread_cond_wait(&changed, &gate);
        }
        pthread_mutex_unlock(&gate);
    }
    launch_check(source->provider == test->expected && !strcmp(source->relative, "dev/keyboard"),
        "namespace change retargeted an admitted source");
    return source->provider == test->routes->provider ?
        dvm_launch_address_create(test->routes->server, "pinned-provider") :
        dvm_launch_address_create(test->routes->client, "new-client");
}


/* EN/RU: Native worker performs the actual route call; no guest, provider parser or result is mocked.
 * Native worker выполняет настоящий route call; guest/provider parser/result не подменяются.
 */
static void *run(void *state)
{
    RouteCase *test = state;
    test->ticket = dvm_launch_route_submit(test->group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, test->spec, locate, test);
    return NULL;
}


/* EN/RU: Replace all live root bindings after metadata selection, then prove old and new submissions go to different owners.
 * Заменить live root bindings после metadata selection, проверить разные destinations старого и нового requests.
 */
int main(void)
{
    LaunchState terminal = {.value = 'T', .terminal = 1};
    LaunchState provider = {.value = 'P'};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_Namespace *group = dvm_launch_prepare(&options);
    DVM_LaunchService *client = dvm_launch_service_create(options.terminal_origin.system, 1);
    DVM_LaunchService *server = dvm_launch_service_create(options.provider_origin.system, 1);
    DVM_LaunchEndpoint *a = dvm_launch_service_endpoint(client);
    DVM_LaunchEndpoint *b = dvm_launch_service_endpoint(server);
    launch_check(group && client && server && a && b, "pinned route fixture");
    Routes routes = {.terminal = options.terminal, .provider = options.provider, .client = a, .server = b};
    DVM_LaunchSpec spec = route_input(&options);
    const unsigned char bytes[] = {0, 255, 128, 1};
    spec.arguments = bytes;
    spec.argument_bytes = sizeof(bytes);
    RouteCase test = {.group = group, .spec = &spec, .routes = &routes, .expected = options.provider, .hold = 1};
    pthread_t worker; /* Native route user joined before namespace/endpoints are freed. */
    launch_check(!pthread_create(&worker, NULL, run, &test), "pinned route worker start");
    pthread_mutex_lock(&gate);
    while (!test.entered)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    launch_check(dvm_stack_mount(group, options.terminal, "/", 0, 0) == 1, "replace selected VFS source during resolver");
    pthread_mutex_lock(&gate);
    test.proceed = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    launch_check(!pthread_join(worker, NULL), "pinned route worker join");
    received(server, test.ticket, "pinned-provider", options.provider_origin.system);
    DVM_LaunchServiceInfo info;
    launch_check(dvm_launch_service_info(client, &info) && !info.outstanding, "in-flight source switched to new owner");
    test.hold = 0;
    test.expected = options.terminal;
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    launch_check(cancel && dvm_cancel_enter(&scope, cancel), "post-ack cancellation scope");
    cancel_after_ack = cancel;
    DVM_LaunchTicket *ticket = dvm_launch_route_submit(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &spec, locate, &test);
    cancel_after_ack = NULL;
    launch_check(ticket && dvm_cancel_requested(cancel) && dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING,
        "acknowledged route ticket was cancelled or discarded");
    launch_check(dvm_cancel_leave(&scope), "post-ack cancellation scope retirement");
    dvm_cancel_release(cancel);
    received(client, ticket, "new-client", options.terminal_origin.system);
    dvm_launch_endpoint_free(a);
    dvm_launch_endpoint_free(b);
    dvm_launch_service_free(client);
    dvm_launch_service_free(server);
    dvm_namespace_release(group);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "pinned route source lifetime drain");
    puts("PASS pinned source routing across bind replacement and acknowledged cancellation");
    return 0;
}
