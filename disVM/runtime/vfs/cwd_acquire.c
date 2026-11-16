/* Copyright (c) 2026 bhgv
 * EN: Native single-directory cwd acquisition with staged provider/anchor ownership.
 * RU: Native захват одиночного cwd с staged владением provider/anchor.
 */
#include "resource.h"
#include "cancel.h"
#include "path_cancel.h"
#include <string.h>


/* EN/RU: Borrow plan through metadata/search/capture; stage owned directory in it.
 * Return one on acquisition, zero on refusal/cancellation; no commit or VM access.
 * Заимствовать plan для metadata/search/capture; сохранить свой staged каталог.
 * Один при успехе, ноль при refusal/cancellation; без commit и VM access. */
int dvm_resource_directory_acquire(DVM_ResourcePlan *plan)
{
    FVM_FileInfo metadata;
    if ((dvm_cancel_current() && !dvm_capture_cancellable(plan)) || plan->union_root || plan->created || plan->created_provider ||
        dvm_cancel_requested(dvm_cancel_current()) ||
        dvm_resource_info(plan, &metadata) != FVM_OK || !metadata.directory ||
        dvm_cancel_requested(dvm_cancel_current()))
    {
        return 0;
    }
    if (!plan->provider)
    {
        plan->created = dvm_anchor_open(plan->anchor, plan->anchor ? plan->relative : plan->names.native);
        return plan->created && !dvm_cancel_requested(dvm_cancel_current());
    }
    if (plan->layers)
    {
        return 0;
    }
    FVM_Result access = dvm_provider_access(plan->provider, plan->relative, FVM_ACCESS_EXEC);
    if ((access != FVM_OK && access != FVM_UNSUPPORTED) || dvm_cancel_requested(dvm_cancel_current()))
    {
        return 0;
    }
    if (dvm_provider_bindable_at(plan->provider, plan->relative))
    {
        if (!dvm_provider_retain(plan->provider))
        {
            return 0;
        }
        plan->created_provider = plan->provider;
        strcpy(plan->created_component, plan->relative);
    }
    else
    {
        plan->created_provider = dvm_provider_capture(plan->provider, plan->relative);
        strcpy(plan->created_component, ".");
    }
    if (!plan->created_provider || dvm_cancel_requested(dvm_cancel_current()))
    {
        return 0;
    }
    if (dvm_provider_info(plan->created_provider, plan->created_component, &metadata) != FVM_OK ||
        !metadata.directory || dvm_cancel_requested(dvm_cancel_current()))
    {
        return 0;
    }
    access = dvm_provider_access(plan->created_provider, plan->created_component, FVM_ACCESS_EXEC);
    return (access == FVM_OK || access == FVM_UNSUPPORTED) && !dvm_cancel_requested(dvm_cancel_current());
}
