/* Copyright (c) 2026 bhgv
 * EN: Actual guest terminal FD operations and live stream discovery after native handoff.
 * RU: Настоящие guest FD operations и live discovery после native handoff.
 */
#include "fixtures/terminal-init/native.h"


/* EN/RU: Prepare/take native launch request before app init and expose its channel grant to capture.
 * Only this fixture chooses /link; production namespace naming is caller-controlled.
 * Подготовить/take launch до init; только fixture выбирает /link для capture capability. */
DVM_Namespace *__wrap_dvm_namespace_rebase(DVM_ResourcePlan *cwd)
{
    (void)cwd;
    terminal.terminal = 1;
    DVM_LaunchOptions options = launch_options(&terminal, &execution);
    DVM_TerminalChannels channels = {0};
    channels.provider = dvm_provider_create_native(&ops, NULL, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    strcpy(channels.receive, "rx");
    strcpy(channels.send, "tx");
    DVM_LaunchSpec spec = {0};
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "terminal-app");
    spec.flashvm = &channels;
    DVM_LaunchRequest *request = NULL;
    DVM_LaunchTicket *ticket = dvm_launch_request_create(&spec, &request);
    launch_check(ticket && dvm_launch_request_prepare(request, options.provider, &options.provider_origin), "terminal launch prepare");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    DVM_NamespacePlan target;
    launch_check(handoff && dvm_namespace_plan(handoff->group, "/link", &target) &&
        dvm_namespace_mount_provider(handoff->group, &target, handoff->flashvm.provider, "/FlashVM") == 1,
        "channel grant exposure for actual guest capture");
    DVM_Namespace *group = handoff->group;
    handoff->group = NULL;
    dvm_launch_handoff_free(handoff);
    dvm_launch_ticket_free(ticket);
    dvm_launch_request_free(request);
    dvm_provider_release(channels.provider);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    return group;
}


/* EN/RU: After real VM GC/close drain, all channel streams and grant roots must retire exactly once.
 * После VM GC/close drain каждый channel stream и grant root должен завершиться ровно один раз. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    launch_check(opened >= 5 && opened == closed && retired == 1 &&
        terminal.destroyed == 1 && execution.destroyed == 1, "terminal FD/root lifetime drain");
    puts("PASS terminal init native lifetimes");
}
