/* Copyright (c) 2026 bhgv
 * EN: One caller-relative VFS selection to a real native launch capability; no code image or automatic retry.
 * RU: Один caller-relative VFS selection в real native launch capability; без code image и automatic retry.
 */
#include "address_internal.h"
#include "../path_cancel.h"
#include "../cancel.h"
#include <string.h>


/* EN/RU: See route.h; source plan owns exact chosen provider/anchor through resolver, submit and address retirement.
 * См. route.h; source plan удерживает exact provider/anchor до конца resolver, submit и address retirement.
 */
DVM_LaunchTicket *dvm_launch_route_submit(DVM_Namespace *group, const char *path, unsigned local_role,
    const DVM_LaunchSpec *spec, DVM_LaunchLocate locate, void *context)
{
    if (!locate || !dvm_launch_route_input(spec) || dvm_cancel_requested(dvm_cancel_current()))
    {
        return NULL;
    }
    DVM_ResourcePlan source;
    if (!dvm_resource_plan_call(group, path, local_role, &source))
    {
        return NULL;
    }
    source.absence_only = 1; /* A call never interprets an opaque lookup error as absence. */
    source.requested_access = FVM_ACCESS_READ | FVM_ACCESS_EXEC;
    FVM_FileInfo metadata; /* Final selected node's acknowledged metadata, never interpreted as an execution grant. */
    DVM_LaunchAddress *address = NULL; /* Owned callback result, consumed even if cancellation races acquisition. */
    DVM_LaunchTicket *ticket = NULL; /* Acknowledged requester result only; no provider review escapes. */
    if ((!dvm_cancel_current() || dvm_path_cancellable(&source)) &&
        dvm_resource_info(&source, &metadata) == FVM_OK && !metadata.directory &&
        !dvm_cancel_requested(dvm_cancel_current()))
    {
        address = locate(context, &source, &metadata);
        if (address && !dvm_cancel_requested(dvm_cancel_current()))
        {
            ticket = dvm_launch_address_submit(address, spec);
        }
    }
    dvm_launch_address_free(address);
    dvm_resource_close(&source);
    return ticket;
}
