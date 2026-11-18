/* Copyright (c) 2026 bhgv
 * EN: Capability addresses, caller-local VFS routing, final refusal and cancellation without implicit code transfer.
 * RU: Capability addresses, caller-local VFS routing, final refusal и cancellation без неявной передачи кода.
 */
#include "test.h"


/* EN/RU: Resolve only an actual pinned source to its owner endpoint; copied labels may differ from caller's virtual path.
 * Разрешить лишь actual pinned source в endpoint владельца; copied labels могут отличаться от virtual path caller.
 */
static DVM_LaunchAddress *locate(void *context, const DVM_ResourcePlan *selected, const FVM_FileInfo *metadata)
{
    Routes *routes = context;
    routes->calls++;
    launch_check(selected->selected && !metadata->directory && (!strcmp(selected->relative, "dev/keyboard") || !strcmp(selected->relative, "server")),
        "resolver did not receive exact selected node");
    if (routes->refuse)
    {
        return NULL;
    }
    DVM_LaunchAddress *address = selected->provider == routes->terminal ?
        dvm_launch_address_create(routes->client, "owner/client-module") : selected->provider == routes->provider ?
        dvm_launch_address_create(routes->server, "owner/provider-module") : NULL;
    if (routes->cancel)
    {
        dvm_cancel_request(routes->cancel);
    }
    return address;
}


/* EN/RU: Require no queued/outstanding request on a borrowed service; no readiness polling or target execution.
 * Требовать пустой queued/outstanding service; без readiness polling и target execution.
 */
static void empty(DVM_LaunchService *owner)
{
    DVM_LaunchServiceInfo info;
    launch_check(dvm_launch_service_info(owner, &info) && !info.outstanding && !info.queued,
        "failed/native route leaked an admission reservation");
}


/* EN/RU: Copy independent address ownership, reject invalid labels, preserve admitted ticket after address retirement.
 * Скопировать independent address ownership, отвергнуть invalid labels, сохранить ticket после address retirement.
 */
static void addresses(DVM_LaunchOptions *options, const DVM_LaunchSpec *spec)
{
    DVM_LaunchService *owner = dvm_launch_service_create(options->provider_origin.system, 2);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    char label[] = "immutable-owner-label"; /* Mutated after address construction to expose borrowed-string bugs. */
    char oversized[DVM_NS_PATH_LIMIT];
    memset(oversized, 'x', sizeof(oversized));
    launch_check(owner && endpoint && !dvm_launch_endpoint_clone(NULL) &&
        !dvm_launch_address_create(endpoint, NULL) && !dvm_launch_address_create(endpoint, "") &&
        !dvm_launch_address_create(endpoint, oversized) && !dvm_launch_address_create(NULL, label), "invalid native address accepted");
    DVM_LaunchAddress *address = dvm_launch_address_create(endpoint, label);
    launch_check(address != NULL, "address construction");
    memset(label, 'z', sizeof(label) - 1);
    dvm_launch_endpoint_free(endpoint);
    DVM_LaunchTicket *ticket = dvm_launch_address_submit(address, spec);
    dvm_launch_address_free(address);
    received(owner, ticket, "immutable-owner-label", options->provider_origin.system);
    empty(owner);
    endpoint = dvm_launch_service_endpoint(owner);
    address = dvm_launch_address_create(endpoint, "already-held");
    launch_check(address && dvm_launch_service_close(owner) && !dvm_launch_endpoint_clone(endpoint) &&
        !dvm_launch_address_create(endpoint, "closed") && !dvm_launch_address_submit(address, spec),
        "closed endpoint accepted native address or work");
    dvm_launch_service_free(owner);
    launch_check(!dvm_launch_address_submit(address, spec), "retired service owner revived through address");
    dvm_launch_address_free(address);
    dvm_launch_endpoint_free(endpoint);
}


/* EN/RU: Construct exact-file alias with normal source provenance, no string-based owner registry.
 * Создать exact-file alias с ordinary source provenance, без string-based owner registry.
 */
static void alias(DVM_Namespace *group)
{
    DVM_ResourcePlan source;
    DVM_ResourcePlan target;
    launch_check(dvm_resource_plan_call(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &source) &&
        dvm_resource_plan(group, "/alias", 0, &target) &&
        dvm_namespace_bind_layers(group, &source, &target, 0, 0, "") == 1, "route exact-file alias");
    dvm_resource_close(&target);
    dvm_resource_close(&source);
}


/* EN/RU: Under a native cancel scope, reject an uninterruptible candidate before its metadata/resolver callback runs.
 * Под native cancel scope отвергнуть uninterruptible candidate до metadata/resolver callback.
 */
static void unsupported(DVM_Namespace *group, const DVM_LaunchSpec *spec, Routes *routes)
{
    DVM_Namespace *copy = dvm_namespace_copy(group);
    LaunchState uninterruptible = {0}; /* Separate destructor/callback counters, not shared with live fixture roots. */
    DVM_Provider *source = dvm_provider_create(&launch_ops, &uninterruptible);
    launch_check(copy && source && copy->bindings->next, "unsupported source fixture");
    dvm_provider_release(copy->bindings->next->provider);
    copy->bindings->next->provider = source; /* Constructor-owned unpublished copy only. */
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    unsigned before = routes->calls;
    launch_check(cancel && dvm_cancel_enter(&scope, cancel), "unsupported source cancellation scope");
    launch_check(!dvm_launch_route_submit(copy, "/denied", DVM_ORIGIN_PROVIDER, spec, locate, routes) &&
        !uninterruptible.denied && routes->calls == before, "uncancellable route performed metadata or resolver I/O");
    launch_check(dvm_cancel_leave(&scope), "unsupported source scope release");
    dvm_cancel_release(cancel);
    dvm_namespace_release(copy);
    launch_check(uninterruptible.destroyed == 1, "unsupported source lifetime drain");
}


/* EN/RU: Exercise real source selection to two independent owner queues, with no module image read or guest callback.
 * Проверить actual source selection в две independent owner queues, без чтения module image и guest callback.
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
    launch_check(group && client && server && a && b, "native route fixture");
    Routes routes = {.terminal = options.terminal, .provider = options.provider, .client = a, .server = b};
    DVM_LaunchSpec spec = route_input(&options);
    const unsigned char bytes[] = {0, 255, 128, 1};
    spec.arguments = bytes;
    spec.argument_bytes = sizeof(bytes);
    addresses(&options, &spec);
    unsupported(group, &spec, &routes);
    for (unsigned role = DVM_ORIGIN_TERMINAL; role <= DVM_ORIGIN_PROVIDER; role++)
    {
        DVM_LaunchTicket *ticket = dvm_launch_route_submit(group, "/dev/keyboard", role, &spec, locate, &routes);
        received(role == DVM_ORIGIN_TERMINAL ? client : server, ticket,
            role == DVM_ORIGIN_TERMINAL ? "owner/client-module" : "owner/provider-module",
            role == DVM_ORIGIN_TERMINAL ? options.terminal_origin.system : options.provider_origin.system);
    }
    DVM_LaunchTicket *fallback = dvm_launch_route_submit(group, "/server", DVM_ORIGIN_TERMINAL, &spec, locate, &routes);
    received(server, fallback, "owner/provider-module", options.provider_origin.system);
    terminal.terminal = 0;
    provider.terminal = 1;
    fallback = dvm_launch_route_submit(group, "/server", DVM_ORIGIN_PROVIDER, &spec, locate, &routes);
    received(client, fallback, "owner/client-module", options.terminal_origin.system);
    terminal.terminal = 1;
    provider.terminal = 0;
    alias(group);
    DVM_LaunchTicket *ticket = dvm_launch_route_submit(group, "/alias", DVM_ORIGIN_TERMINAL, &spec, locate, &routes);
    received(server, ticket, "owner/provider-module", options.provider_origin.system);
    unsigned before = routes.calls;
    routes.refuse = 1;
    launch_check(!dvm_launch_route_submit(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &spec, locate, &routes) &&
        routes.calls == before + 1, "resolver refusal retried another candidate");
    routes.refuse = 0;
    before = routes.calls;
    launch_check(!dvm_launch_route_submit(group, "/denied", DVM_ORIGIN_TERMINAL, &spec, locate, &routes) &&
        !dvm_launch_route_submit(group, "/dev", DVM_ORIGIN_TERMINAL, &spec, locate, &routes) &&
        !dvm_launch_route_submit(group, "/missing", DVM_ORIGIN_TERMINAL, &spec, locate, &routes) &&
        routes.calls == before, "denied/missing/directory node reached launch resolver");
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    launch_check(cancel && dvm_cancel_enter(&scope, cancel), "route cancellation scope");
    routes.cancel = cancel;
    launch_check(!dvm_launch_route_submit(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &spec, locate, &routes) &&
        routes.calls == before + 1, "resolver-time cancellation admitted work");
    before = routes.calls;
    launch_check(!dvm_launch_route_submit(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &spec, locate, &routes) &&
        routes.calls == before, "pre-cancel invoked resolver");
    launch_check(dvm_cancel_leave(&scope), "route cancellation scope release");
    dvm_cancel_release(cancel);
    routes.cancel = NULL;
    before = routes.calls;
    size_t length = spec.argument_bytes;
    spec.argument_bytes = DVM_LAUNCH_ARGUMENT_LIMIT + 1;
    launch_check(!dvm_launch_route_submit(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &spec, locate, &routes) &&
        routes.calls == before, "invalid request reached resolver before preflight");
    spec.argument_bytes = length;
    ticket = dvm_launch_route_submit(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &spec, locate, &routes);
    launch_check(ticket && !dvm_launch_route_submit(group, "/dev/keyboard", DVM_ORIGIN_PROVIDER, &spec, locate, &routes),
        "route bypassed bounded destination admission");
    received(server, ticket, "owner/provider-module", options.provider_origin.system);
    empty(client);
    empty(server);
    dvm_launch_endpoint_free(a);
    dvm_launch_endpoint_free(b);
    dvm_launch_service_free(client);
    dvm_launch_service_free(server);
    dvm_namespace_release(group);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "route provider lifetime drain");
    puts("PASS caller-local VFS routing to immutable native launch addresses");
    return 0;
}
