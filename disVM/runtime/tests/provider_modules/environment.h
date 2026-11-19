/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real per-instance environment attachment, dependency calls, pctl and native retirement acceptance.
 * RU: Настоящие per-instance attachment, dependency calls, pctl и native retirement.
 */
#ifndef PROVIDER_MODULE_ENVIRONMENT_TEST_H
#define PROVIDER_MODULE_ENVIRONMENT_TEST_H
#include "../../module_interface/environment.h"
#include "../../vfs/launch/context.h"
#include "../../vfs/boot.h"
#include <limits.h>
static DVM_Namespace *prepared_environments[2]; /* Controller prepares each instance once before admitting any guest. */
static DVM_LaunchContext *prepared_contexts[2]; /* Independent copied snapshots, caller pins dropped immediately after attachment. */
static unsigned environment_bindings; /* Actual native attachment count, read only after guest retirement. */


/* EN: Explicitly grant the actual boot event device into an unpublished fixture group; no path or recipient is mocked.
 * RU: Явно предоставить настоящее boot-устройство событий в неопубликованную группу теста, без подмены получателя.
 */
static void environment_events(DVM_Namespace *group)
{
    DVM_ResourcePlan source;
    DVM_NamespacePlan target;
    launch_check(dvm_resource_plan(dvm_namespace_boot_group(), "/dev/sysevents", 0, &source) && source.provider,
        "actual boot event device grant");
    launch_check(dvm_namespace_plan(group, "/dev/sysevents", &target) &&
        dvm_namespace_mount_provider(group, &target, source.provider, "/events-grant") == 1,
        "explicit fixture event node binding");
    dvm_resource_close(&source);
}


/* EN/RU: Prepare independent owner-first live groups on provider controller, borrowing granted options only here.
 * Подготовить independent owner-first live groups на provider controller; options borrowed лишь до return.
 */
static void environment_prepare(const DVM_LaunchOptions *options, const DVM_LaunchHandoff *handoff)
{
    if (CONTROL >= 25)
    {
        if (CONTROL >= 39)
        {
            environment_events(handoff->group);
        }
        DVM_LaunchOptions owned = *options;
        owned.policy = DVM_LAUNCH_OWNER_FIRST;
        unsigned count = (CONTROL == 27 || CONTROL == 32 || CONTROL == 40) ? 2 : 1;
        for (unsigned i = 0; i < count; i++)
        {
            prepared_environments[i] = dvm_launch_prepare(&owned);
            launch_check(prepared_environments[i] != NULL, "per-instance owner-first preparation");
            if (CONTROL >= 39)
            {
                environment_events(prepared_environments[i]);
            }
            if (CONTROL >= 29)
            {
                DVM_LaunchHandoff selected = *handoff; /* Trusted owner fixture chooses immutable invocation payload after admission. */
                unsigned char arguments[3] = {77, 0, (unsigned char)(i + 1)};
                selected.description.argument_bytes = sizeof(arguments);
                selected.arguments = arguments;
                prepared_contexts[i] = dvm_launch_context_create(&selected);
                launch_check(prepared_contexts[i] != NULL, "per-instance native context snapshot");
            }
        }
    }
}


/* [GC:] EN/RU: Attach only the actual newly linked app instance before returning it to mload; no guest lookup is mocked.
 * Подключить actual linked app instance до возврата mload, не подменяя guest lookup или execution.
 */
static void environment_attach(Modlink *link)
{
    if (CONTROL < 25 || !currun() || !link || link == H || strcmp(link->m->path, "/app.dis"))
    {
        return;
    }
    unsigned index = environment_bindings;
    launch_check(index < ((CONTROL == 27 || CONTROL == 32 || CONTROL == 40) ? 2U : 1U) && prepared_environments[index], "one prepared group per instance");
    DVM_LaunchContext *context = prepared_contexts[index];
    if (context)
    {
        unsigned original = context->references; /* Still private, not yet attached/published by this owner. */
        context->references = UINT_MAX;
        launch_check(!dvm_module_environment_bind_context(link, prepared_environments[index], context),
            "overflowed native context reference was accepted");
        context->references = original;
    }
    launch_check(dvm_module_environment_bind_context(link, prepared_environments[index], prepared_contexts[index]),
        "actual private instance metadata/namespace attachment");
    launch_check(!dvm_module_environment_bind_context(link, prepared_environments[index], context),
        "duplicate module metadata attachment accepted");
    launch_check(!context || context->references == 2, "duplicate attachment lost or leaked context ownership");
    dvm_launch_context_release(prepared_contexts[index]);
    prepared_contexts[index] = NULL;
    dvm_namespace_release(prepared_environments[index]);
    prepared_environments[index] = NULL;
    environment_bindings++;
}
#endif
