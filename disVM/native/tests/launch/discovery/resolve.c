/* Copyright (c) 2026 bhgv
 * EN: Deferred caller-local module resolution, concurrent reuse and independence from later namespace changes.
 * RU: Отложенный caller-local resolve модуля, concurrent reuse и независимость от последующих смен namespace.
 */
#include "source.h"
#include <pthread.h>


/* EN/RU: Each native worker borrows one shared immutable address and its own request through join.
 * Каждый native worker заимствует общий immutable address и собственный request до join.
 */
typedef struct Submit
{
    DVM_LaunchAddress *address; /* Borrowed shared address, main frees only after both workers join. */
    DVM_LaunchSpec request; /* Per-worker borrowed grants and copied binary input descriptor. */
    DVM_LaunchTicket *ticket; /* Owned real queue acknowledgement moved to main after join. */
} Submit;


/* EN/RU: Submit once on a dedicated native test thread; return NULL, main owns the resulting ticket.
 * Один submit на dedicated native test thread; return NULL, main владеет resulting ticket.
 */
static void *submit(void *state)
{
    Submit *work = state;
    work->ticket = dvm_launch_address_submit(work->address, &work->request);
    return NULL;
}


/* EN/RU: Inspect both actual requests before freeing unmatched requester tickets; concurrent arrival order is unspecified.
 * Require isolated binary payloads from workers; consume reviews/tickets only after both are acquired.
 * Проверить оба real requests до освобождения tickets: concurrent arrival order не задан.
 * Требовать изоляцию binary payloads; поглотить reviews/tickets после получения обоих.
 */
static void received_pair(DVM_LaunchService *owner, Submit jobs[2])
{
    DVM_LaunchReview *reviews[2];
    unsigned seen = 0;
    for (unsigned i = 0; i < 2; i++)
    {
        reviews[i] = dvm_launch_service_next(owner);
        DVM_LaunchDescription description;
        unsigned char payload[4];
        size_t copied = 0;
        DVM_LaunchRequest *request = dvm_launch_review_request(reviews[i]);
        launch_check(reviews[i] && dvm_launch_describe(request, &description) &&
            !strcmp(description.module, "owned/dev/keyboard") &&
            dvm_launch_arguments(request, 0, payload, sizeof(payload), &copied) && copied == 4 &&
            payload[0] < 2 && payload[1] == 255 && payload[2] == 0 && payload[3] == 128 &&
            !(seen & (1U << payload[0])), "concurrent reusable address mixed or lost request arguments");
        seen |= 1U << payload[0];
    }
    launch_check(seen == 3, "concurrent reusable address missed one caller");
    for (unsigned i = 0; i < 2; i++)
    {
        dvm_launch_review_free(reviews[i]);
        dvm_launch_ticket_free(jobs[i].ticket);
    }
}


/* EN/RU: Require exact real queue counts without consuming requests; owner borrowed, no guest or provider callback.
 * Проверить точные real queue counts без поглощения requests; owner borrowed, без guest/provider callback.
 */
static void queued(DVM_LaunchService *owner, unsigned count)
{
    DVM_LaunchServiceInfo info;
    launch_check(dvm_launch_service_info(owner, &info) && info.queued == count && info.outstanding == count,
        "resolution reserved quota or submission reached the wrong owner");
}


/* EN/RU: Native cancellation before/after callback refuses owned resolution without leaking queue slots or poisoning the source.
 * Native cancellation до/после callback отказывает owned resolution без queue leaks и повреждения source.
 */
static void cancelled(DVM_Namespace *group, SourceStats *stats, DVM_LaunchService *owner)
{
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    launch_check(cancel && dvm_cancel_enter(&scope, cancel), "deferred resolution scope");
    stats->cancel_on_query = cancel;
    unsigned before = stats->queries;
    launch_check(!dvm_launch_route_resolve(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER) &&
        dvm_cancel_requested(cancel) && stats->queries == before + 1, "late cancellation escaped an owned address");
    before = stats->queries;
    launch_check(!dvm_launch_route_resolve(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER) && stats->queries == before,
        "pre-cancelled resolution invoked a provider");
    queued(owner, 0);
    stats->cancel_on_query = NULL;
    launch_check(dvm_cancel_leave(&scope), "deferred resolution scope retirement");
    dvm_cancel_release(cancel);
}


/* EN/RU: Retain an export address after namespace retirement; recheck current execute policy on every later submit.
 * Удержать export address после retirement namespace; проверять current execute policy при каждом later submit.
 */
static void policy(DVM_LaunchOptions *options, DVM_Provider *source, DVM_LaunchService *owner,
    const DVM_LaunchSpec *request)
{
    Policy decision = {.allowed = 1};
    DVM_View *builder = dvm_view_create(policy_check, &decision, policy_release);
    launch_check(builder && dvm_view_map(builder, "/run", source, "dev/keyboard", FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0),
        "deferred export builder");
    DVM_Provider *view = dvm_view_freeze(builder);
    DVM_Namespace *group = dvm_launch_prepare(options);
    launch_check(view && group && dvm_stack_mount(group, view, "/", 0, 0) == 1, "deferred export namespace");
    DVM_LaunchAddress *address = dvm_launch_route_resolve(group, "/run", DVM_ORIGIN_PROVIDER);
    launch_check(address != NULL, "deferred filtered address");
    dvm_namespace_release(group);
    dvm_provider_release(view);
    launch_check(!decision.released, "resolution lost retained live policy");
    __atomic_store_n(&decision.allowed, 0, __ATOMIC_RELEASE);
    launch_check(!dvm_launch_address_submit(address, request), "deferred address bypassed revoked execute policy");
    queued(owner, 0);
    __atomic_store_n(&decision.allowed, 1, __ATOMIC_RELEASE);
    source_received(owner, dvm_launch_address_submit(address, request));
    dvm_launch_address_free(address);
    launch_check(decision.released == 1, "deferred address leaked export policy");
}


/* EN/RU: Bind both caller sides without arguments/admission, replace namespace, then reuse only the selected native owner.
 * Resolve обеих caller sides без arguments/admission, смена namespace и reuse только выбранного native owner.
 */
int main(void)
{
    LaunchState terminal = {.terminal = 1};
    LaunchState provider = {0};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    SourceStats a = {0};
    SourceStats b = {0};
    DVM_LaunchService *client = dvm_launch_service_create(options.terminal_origin.system, 2);
    DVM_LaunchService *server = dvm_launch_service_create(options.provider_origin.system, 2);
    DVM_LaunchEndpoint *left = dvm_launch_service_endpoint(client);
    DVM_LaunchEndpoint *right = dvm_launch_service_endpoint(server);
    options.terminal = source_create(left, &a, ".", SOURCE_FLAGS, 1);
    options.provider = source_create(right, &b, ".", SOURCE_FLAGS, 1);
    DVM_Namespace *group = dvm_launch_prepare(&options);
    launch_check(client && server && left && right && group, "deferred resolution fixture");
    unsigned before = a.queries + b.queries;
    launch_check(!dvm_launch_route_resolve(NULL, "/dev/keyboard", DVM_ORIGIN_PROVIDER) &&
        !dvm_launch_route_resolve(group, NULL, DVM_ORIGIN_PROVIDER) &&
        !dvm_launch_route_resolve(group, "/dev/keyboard", 0) &&
        !dvm_launch_route_resolve(group, "/missing", DVM_ORIGIN_PROVIDER) &&
        !dvm_launch_route_resolve(group, "/dev", DVM_ORIGIN_PROVIDER) &&
        a.queries + b.queries == before, "invalid/missing/directory resolution invoked an advertisement");
    DVM_LaunchAddress *local = dvm_launch_route_resolve(group, "/dev/keyboard", DVM_ORIGIN_TERMINAL);
    DVM_LaunchAddress *remote = dvm_launch_route_resolve(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER);
    launch_check(local && remote && a.queries == 1 && b.queries == 1, "deferred lookup lost caller-local order");
    queued(client, 0);
    queued(server, 0);
    cancelled(group, &b, server);
    DVM_LaunchSpec request = {0};
    request.policy = options.policy;
    request.terminal = options.terminal;
    request.terminal_origin = options.terminal_origin;
    policy(&options, options.provider, server, &request);
    launch_check(dvm_stack_mount(group, options.terminal, "/", 0, 0) == 1, "replace namespace after resolution");
    DVM_LaunchAddress *replacement = dvm_launch_route_resolve(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER);
    launch_check(replacement != NULL && a.queries == 2, "new resolution ignored changed binding");
    dvm_namespace_release(group);
    dvm_provider_release(options.provider);
    launch_check(b.created == b.destroyed, "raw native address unnecessarily retained caller/source factory");
    dvm_launch_endpoint_free(right);
    unsigned queries = a.queries + b.queries;
    unsigned metadata = a.metadata + b.metadata;
    Submit jobs[2] = {{.address = remote, .request = request}, {.address = remote, .request = request}};
    const unsigned char arguments[2][4] = {{0, 255, 0, 128}, {1, 255, 0, 128}}; /* Distinct native request payloads. */
    pthread_t workers[2]; /* Native submitters, no managed guest objects or cooperative servicing. */
    for (unsigned i = 0; i < 2; i++)
    {
        jobs[i].request.arguments = arguments[i];
        jobs[i].request.argument_bytes = sizeof(arguments[i]);
        launch_check(!pthread_create(&workers[i], NULL, submit, &jobs[i]), "concurrent retained submit start");
    }
    for (unsigned i = 0; i < 2; i++)
    {
        launch_check(!pthread_join(workers[i], NULL) && jobs[i].ticket, "concurrent retained submit join");
    }
    queued(server, 2);
    queued(client, 0);
    launch_check(!dvm_launch_address_submit(remote, &request), "retained address bypassed owner quota");
    received_pair(server, jobs);
    source_received(server, dvm_launch_address_submit(remote, &request));
    source_received(client, dvm_launch_address_submit(local, &request));
    source_received(client, dvm_launch_address_submit(replacement, &request));
    launch_check(queries == a.queries + b.queries && metadata == a.metadata + b.metadata && !a.opened && !b.opened,
        "using a resolved address repeated provider lookup or read code bytes");
    dvm_launch_service_free(server);
    launch_check(!dvm_launch_address_submit(remote, &request), "retained address revived a retired service");
    dvm_launch_address_free(remote);
    dvm_launch_address_free(local);
    dvm_launch_address_free(replacement);
    dvm_launch_endpoint_free(left);
    dvm_launch_service_free(client);
    dvm_provider_release(options.terminal);
    launch_check(a.created == a.destroyed && b.created == b.destroyed, "deferred source ownership drain");
    puts("PASS deferred caller-local resolution, concurrent reuse and bind-independent owner lifetime");
    return 0;
}
