/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real per-instance environment attachment, dependency calls, pctl and native retirement acceptance.
 * RU: Настоящие per-instance attachment, dependency calls, pctl и native retirement.
 */
#ifndef PROVIDER_MODULE_ENVIRONMENT_TEST_H
#define PROVIDER_MODULE_ENVIRONMENT_TEST_H
#include "../../module_interface/environment.h"
static DVM_Namespace *prepared_environments[2]; /* Controller prepares each instance once before admitting any guest. */
static unsigned environment_bindings; /* Actual native attachment count, read only after guest retirement. */


/* EN/RU: Prepare independent owner-first live groups on provider controller, borrowing granted options only here.
 * Подготовить independent owner-first live groups на provider controller; options borrowed лишь до return.
 */
static void environment_prepare(const DVM_LaunchOptions *options)
{
    if (CONTROL >= 25)
    {
        DVM_LaunchOptions owned = *options;
        owned.policy = DVM_LAUNCH_OWNER_FIRST;
        unsigned count = CONTROL == 27 ? 2 : 1;
        for (unsigned i = 0; i < count; i++)
        {
            prepared_environments[i] = dvm_launch_prepare(&owned);
            launch_check(prepared_environments[i] != NULL, "per-instance owner-first preparation");
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
    launch_check(index < (CONTROL == 27 ? 2U : 1U) && prepared_environments[index], "one prepared group per instance");
    launch_check(dvm_module_environment_bind(link, prepared_environments[index]), "actual private instance attachment");
    launch_check(!dvm_module_environment_bind(link, prepared_environments[index]), "duplicate module attachment accepted");
    dvm_namespace_release(prepared_environments[index]);
    prepared_environments[index] = NULL;
    environment_bindings++;
}
#endif
