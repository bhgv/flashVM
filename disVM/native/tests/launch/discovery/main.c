/* Copyright (c) 2026 bhgv
 * EN: Automatic advertised-address discovery, scoped capture, filtered exports and live execution policy after acquisition.
 * RU: Автоматический advertised-address discovery, scoped capture, filtered exports и live execution policy после acquisition.
 */
#include "source.h"


/* EN/RU: Build caller-owned request grants with deliberately irrelevant destination labels; address supplies actual owner.
 * Сформировать caller request grants с deliberately irrelevant destination labels; address задаёт owner.
 */
static DVM_LaunchSpec spec(DVM_LaunchOptions *options)
{
    DVM_LaunchSpec request = {0};
    request.policy = options->policy;
    request.terminal = options->terminal;
    request.terminal_origin = options->terminal_origin;
    strcpy(request.module, "caller-label");
    strcpy(request.expected_provider, options->terminal_origin.system);
    return request;
}


/* EN/RU: Native guard check for the deep ownership test; borrowed policy,1/0, no allocation or I/O.
 * Native guard check для deep ownership test; borrowed policy,1/0, без allocation/I/O.
 */
static int allowed(void *state)
{
    Policy *policy = state;
    return __atomic_load_n(&policy->allowed, __ATOMIC_ACQUIRE);
}


/* EN/RU: Deep guard chains must submit/free iteratively and preserve exact permission state without consuming wrappers.
 * Deep guard chains выполняют submit/free итеративно, сохраняя permission state и wrappers.
 */
static void deep(DVM_LaunchEndpoint *endpoint, DVM_LaunchService *owner, const DVM_LaunchSpec *request)
{
    Policy policy = {.allowed = 1};
    DVM_LaunchAddress *address = dvm_launch_address_create(endpoint, "owned/dev/keyboard");
    launch_check(address != NULL, "deep guard leaf");
    for (unsigned i = 0; i < 5000; i++)
    {
        address = dvm_launch_address_guard(address, allowed, &policy, NULL);
        launch_check(address != NULL, "deep guard construction");
    }
    source_received(owner, dvm_launch_address_submit(address, request));
    __atomic_store_n(&policy.allowed, 0, __ATOMIC_RELEASE);
    launch_check(!dvm_launch_address_submit(address, request), "guard chain ignored changed policy");
    dvm_launch_address_free(address);
}


/* EN/RU: Legacy/unscoped callbacks cannot silently claim scope cancellation, canonical paths reject before callback.
 * Legacy/unscoped callbacks не получают scope cancellation неявно; canonical paths отказывают до callback.
 */
static void promises(DVM_LaunchEndpoint *endpoint, SourceStats *stats, const DVM_LaunchSpec *request, DVM_LaunchService *owner)
{
    DVM_Provider *legacy = source_create(endpoint, stats, ".", DVM_PROVIDER_CANCEL_PATH, 0);
    DVM_Provider *unscoped = source_create(endpoint, stats, ".", DVM_PROVIDER_CANCEL_PATH, 1);
    launch_check(legacy && unscoped && !source_create(endpoint, stats, ".", DVM_PROVIDER_CANCEL_LAUNCH, 1) &&
        !source_create(endpoint, stats, ".", DVM_PROVIDER_CANCEL_LAUNCH | DVM_PROVIDER_CANCEL_PATH, 0) &&
        !source_create(endpoint, stats, ".", 16U, 1), "invalid provider launch promises accepted");
    unsigned before = stats->queries;
    launch_check(!dvm_provider_launch(legacy, "dev/keyboard") && !dvm_provider_launch(unscoped, "../dev/keyboard") &&
        !dvm_provider_launch(unscoped, "/dev/keyboard") && !dvm_provider_launch(unscoped, "dev//keyboard") &&
        !dvm_provider_launch(unscoped, "dev/./keyboard") && !dvm_provider_launch(unscoped, "dev/keyboard/") &&
        stats->queries == before, "unsupported/invalid discovery invoked callback");
    DVM_LaunchAddress *address = dvm_provider_launch(unscoped, "dev/keyboard");
    launch_check(address != NULL, "explicit unscoped discovery");
    source_received(owner, dvm_launch_address_submit(address, request));
    dvm_launch_address_free(address);
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    launch_check(cancel && dvm_cancel_enter(&scope, cancel), "provider discovery cancellation scope");
    before = stats->queries;
    launch_check(!dvm_provider_launch(unscoped, "dev/keyboard") && stats->queries == before,
        "PATH promise incorrectly authorized uncancellable launch callback");
    launch_check(dvm_cancel_leave(&scope), "provider discovery scope release");
    dvm_cancel_release(cancel);
    dvm_provider_release(unscoped);
    dvm_provider_release(legacy);
}


/* EN/RU: Mixed views promise cancellable dispatch, but never promote an inner callback lacking LAUNCH opt-in.
 * Mixed views обещают cancellable dispatch, но не повышают права inner callback без LAUNCH opt-in.
 */
static void scoped(DVM_LaunchEndpoint *endpoint, SourceStats *stats, const DVM_LaunchSpec *request, DVM_LaunchService *owner)
{
    DVM_Provider *good = source_create(endpoint, stats, ".", SOURCE_FLAGS, 1);
    DVM_Provider *bad = source_create(endpoint, stats, ".", DVM_PROVIDER_CANCEL_PATH, 1);
    DVM_View *builder = dvm_view_create(NULL, NULL, NULL);
    launch_check(good && bad && builder &&
        dvm_view_map(builder, "/good", good, "dev/keyboard", FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0) &&
        dvm_view_map(builder, "/bad", bad, "dev/keyboard", FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0), "mixed discovery builder");
    DVM_Provider *view = dvm_view_freeze(builder);
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    launch_check(view && cancel && dvm_cancel_enter(&scope, cancel), "mixed discovery scope");
    DVM_LaunchAddress *address = dvm_provider_launch(view, "good");
    launch_check(address != NULL, "mixed view lost cancellable advertised entry");
    source_received(owner, dvm_launch_address_submit(address, request));
    dvm_launch_address_free(address);
    unsigned before = stats->queries;
    launch_check(!dvm_provider_launch(view, "bad") && stats->queries == before,
        "view inferred inner launch cancellation from PATH");
    stats->cancel_on_query = cancel;
    address = dvm_provider_launch(view, "good");
    launch_check(address && dvm_cancel_requested(cancel), "late cancelled discovery did not preserve owned result");
    dvm_launch_address_free(address);
    stats->cancel_on_query = NULL;
    launch_check(dvm_cancel_leave(&scope), "mixed discovery scope retirement");
    dvm_cancel_release(cancel);
    cancel = dvm_cancel_create();
    launch_check(cancel && dvm_cancel_enter(&scope, cancel), "routed discovery scope");
    LaunchState terminal = {.terminal = 1};
    LaunchState provider = {0};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_Namespace *group = dvm_launch_prepare(&options);
    launch_check(group && dvm_stack_mount(group, view, "/", 0, 0) == 1, "cancelled discovery namespace");
    stats->cancel_on_query = cancel;
    launch_check(!dvm_launch_route_discover(group, "/good", DVM_ORIGIN_PROVIDER, request),
        "discovery cancellation before submit admitted work");
    DVM_LaunchServiceInfo info;
    launch_check(dvm_launch_service_info(owner, &info) && !info.outstanding && !info.queued,
        "cancelled discovery left native admission");
    stats->cancel_on_query = NULL;
    launch_check(dvm_cancel_leave(&scope), "routed discovery scope retirement");
    dvm_cancel_release(cancel);
    dvm_namespace_release(group);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    dvm_provider_release(view);
    dvm_provider_release(good);
    dvm_provider_release(bad);
}


/* EN/RU: Expose one module through several alias forms, with hidden/readonly/synthetic nodes and an ongoing policy.
 * Открыть module через разные aliases с hidden/readonly/synthetic nodes и ongoing policy.
 */
static DVM_Provider *exported(DVM_Provider *source, Policy *policy)
{
    DVM_View *view = dvm_view_create(policy_check, policy, policy_release);
    unsigned access = FVM_ACCESS_READ | FVM_ACCESS_EXEC;
    launch_check(view && dvm_view_map(view, "/tools", source, "dev", access, DVM_VIEW_TREE) &&
        dvm_view_map(view, "/data", source, "dev/keyboard", FVM_ACCESS_READ, 0) &&
        dvm_view_map(view, "/hidden", NULL, ".", 0, DVM_VIEW_TREE) &&
        dvm_view_map(view, "/single", source, "dev/keyboard", access, 0) &&
        dvm_view_map(view, "/synthetic/leaf", source, "dev/keyboard", access, 0), "launch export builder");
    return dvm_view_freeze(view);
}


/* EN/RU: Run actual native provider/view discovery and owner queues; no source image or guest result is fabricated.
 * Выполнить actual native provider/view discovery и owner queues; source image/guest result не подменяются.
 */
int main(void)
{
    LaunchState terminal = {.terminal = 1};
    LaunchState provider = {0};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_LaunchSpec request = spec(&options);
    DVM_LaunchService *owner = dvm_launch_service_create(options.provider_origin.system, 2);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    SourceStats stats = {0};
    launch_check(owner && endpoint, "provider discovery service");
    promises(endpoint, &stats, &request, owner);
    deep(endpoint, owner, &request);
    scoped(endpoint, &stats, &request, owner);
    DVM_Provider *source = source_create(endpoint, &stats, ".", SOURCE_FLAGS, 1);
    launch_check(source != NULL, "advertised source");
    DVM_Provider *file = dvm_provider_capture(source, "dev/keyboard");
    launch_check(file != NULL, "scoped advertised file capture");
    DVM_LaunchAddress *address = dvm_provider_launch(file, ".");
    launch_check(address && !dvm_provider_launch(file, "../keyboard"), "captured advertised root escaped");
    source_received(owner, dvm_launch_address_submit(address, &request));
    dvm_launch_address_free(address);
    dvm_provider_release(file);
    Policy policy = {.allowed = 1};
    DVM_Provider *view = exported(source, &policy);
    launch_check(view != NULL, "frozen launch view");
    unsigned before = stats.queries;
    launch_check(!dvm_provider_launch(view, "hidden") && !dvm_provider_launch(view, "data") &&
        !dvm_provider_launch(view, "synthetic") && !dvm_provider_launch(view, "single/child") && stats.queries == before,
        "filtered export delegated hidden/readonly/synthetic/descendant launch");
    address = dvm_provider_launch(view, "tools/keyboard");
    launch_check(address != NULL, "filtered module advertisement");
    __atomic_store_n(&policy.allowed, 0, __ATOMIC_RELEASE);
    before = stats.queries;
    launch_check(!dvm_launch_address_submit(address, &request) && !dvm_provider_launch(view, "tools/keyboard") &&
        stats.queries == before, "acquired view address bypassed changed execute policy");
    __atomic_store_n(&policy.allowed, 1, __ATOMIC_RELEASE);
    source_received(owner, dvm_launch_address_submit(address, &request));
    dvm_launch_address_free(address);
    DVM_Namespace *group = dvm_launch_prepare(&options);
    launch_check(group && dvm_stack_mount(group, view, "/", 0, 0) == 1, "advertised view mount");
    source_received(owner, dvm_launch_route_discover(group, "/tools/keyboard", DVM_ORIGIN_PROVIDER, &request));
    dvm_namespace_release(group);
    DVM_Provider *captured = dvm_provider_capture(view, "tools");
    launch_check(captured != NULL, "scoped view advertisement capture");
    address = dvm_provider_launch(captured, "keyboard");
    launch_check(address != NULL, "captured view address");
    dvm_provider_release(captured);
    dvm_provider_release(view);
    dvm_provider_release(source);
    launch_check(!policy.released && stats.created > stats.destroyed, "guard lost policy/source lifetime after unmount");
    __atomic_store_n(&policy.allowed, 0, __ATOMIC_RELEASE);
    launch_check(!dvm_launch_address_submit(address, &request), "captured address lost original live policy");
    __atomic_store_n(&policy.allowed, 1, __ATOMIC_RELEASE);
    source_received(owner, dvm_launch_address_submit(address, &request));
    dvm_launch_address_free(address);
    launch_check(policy.released == 1 && stats.created == stats.destroyed, "guard/source/context ownership drain");
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(owner);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "caller root lifetime drain");
    puts("PASS provider advertisements, filtered discovery and retained live policy");
    return 0;
}
