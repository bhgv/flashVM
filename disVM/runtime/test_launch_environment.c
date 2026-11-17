/* Copyright (c) 2026 bhgv
 * EN: Actual guest pre-init environment fixture using the production native preparation SDK.
 * RU: Fixture pre-init окружения настоящего guest через production native SDK подготовки.
 */
#include "../native/tests/launch/backend.h"
#include "vfs/launch/service.h"
static LaunchState terminal; /* Native fixture root, alive through final namespace retirement. */
static LaunchState provider; /* Native execution-side root, never guest memory. */
void __real_dvm_namespace_boot_shutdown(void);


/* EN/RU: Replace only the fixture bootstrap's NEWNS result, before application init.
 * ACTIVE bootstrap owns cwd; fixed in-memory roots perform no blocking I/O here.
 * Вернуть prepared namespace вместо fixture NEWNS до application init;
 * ACTIVE bootstrap владеет cwd, memory roots здесь не выполняют blocking I/O. */
DVM_Namespace *__wrap_dvm_namespace_rebase(DVM_ResourcePlan *cwd)
{
    (void)cwd;
    terminal.value = 'T';
    terminal.terminal = 1;
    provider.value = 'P';
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_LaunchSpec spec = {0};
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "fixture-appinit");
    spec.arguments = "init";
    spec.argument_bytes = 4;
    DVM_LaunchService *owner = dvm_launch_service_create(options.provider_origin.system, 4);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(owner);
    DVM_LaunchTicket *ticket = dvm_launch_service_submit(endpoint, &spec);
    DVM_LaunchReview *review = dvm_launch_service_next(owner);
    DVM_LaunchRequest *request = dvm_launch_review_request(review);
    launch_check(ticket && dvm_launch_request_prepare(request, options.provider, &options.provider_origin),
                 "request acceptance before application init");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    launch_check(handoff && dvm_launch_ticket_status(ticket) == DVM_LAUNCH_TAKEN &&
        !dvm_launch_request_take(request) && !memcmp(handoff->arguments, "init", 4), "one-use init handoff");
    dvm_launch_ticket_free(ticket);
    dvm_launch_review_free(review);
    dvm_launch_service_free(owner);
    dvm_launch_endpoint_free(endpoint);
    DVM_Namespace *group = handoff->group;
    handoff->group = NULL;
    dvm_launch_handoff_free(handoff);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    return group;
}


/* EN/RU: Real shutdown drains native streams/roots; all granted roots retire exactly once.
 * Настоящий shutdown дренирует streams/roots; каждый granted root завершён один раз. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "guest launch root retirement");
    puts("PASS guest launch native ownership");
}
