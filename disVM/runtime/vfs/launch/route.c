/* Copyright (c) 2026 bhgv
 * EN: One caller-relative VFS selection to a real native launch capability; no code image or automatic retry.
 * RU: Один caller-relative VFS selection в real native launch capability; без code image и automatic retry.
 */
#include "address_internal.h"
#include "../path_cancel.h"
#include "../cancel.h"
#include <string.h>


/* EN/RU: Select into caller-owned zeroed plan, obtain owned address/NULL; caller always closes plan.
 * Borrow group/path/locate/context through return. No code reads or managed entry; selected source stays pinned.
 * Выбрать в нулевой plan caller, получить свой address/NULL; caller всегда закрывает plan.
 * Inputs borrowed до return; без чтения кода/managed entry; выбранный source удерживается.
 */
static DVM_LaunchAddress *select_address(DVM_Namespace *group, const char *path, unsigned local_role,
    DVM_LaunchLocate locate, void *context, DVM_ResourcePlan *source)
{
    if (!locate || dvm_cancel_requested(dvm_cancel_current()) ||
        !dvm_resource_plan_call(group, path, local_role, source))
    {
        return NULL;
    }
    source->absence_only = 1; /* Opaque lookup failure never means absence. */
    source->requested_access = FVM_ACCESS_READ | FVM_ACCESS_EXEC;
    FVM_FileInfo metadata; /* Actual selected node; metadata is not an execution grant. */
    if ((!dvm_cancel_current() || dvm_path_cancellable(source)) &&
        dvm_resource_info(source, &metadata) == FVM_OK && !metadata.directory &&
        !dvm_cancel_requested(dvm_cancel_current()))
    {
        return locate(context, source, &metadata);
    }
    return NULL;
}


/* EN/RU: See route.h; keep exact chosen provider/anchor pinned through resolver, submit and address retirement.
 * См. route.h; удержать exact provider/anchor до конца resolver, submit и address retirement.
 */
DVM_LaunchTicket *dvm_launch_route_submit(DVM_Namespace *group, const char *path, unsigned local_role,
    const DVM_LaunchSpec *spec, DVM_LaunchLocate locate, void *context)
{
    if (!dvm_launch_route_input(spec))
    {
        return NULL;
    }
    DVM_ResourcePlan source = {0};
    DVM_LaunchAddress *address = select_address(group, path, local_role, locate, context, &source);
    DVM_LaunchTicket *ticket = NULL; /* Only an actual acknowledged requester ticket escapes. */
    if (address && !dvm_cancel_requested(dvm_cancel_current()))
    {
        ticket = dvm_launch_address_submit(address, spec);
    }
    dvm_launch_address_free(address);
    dvm_resource_close(&source);
    return ticket;
}


/* EN/RU: Borrow selected plan/metadata during discovery; return provider's owned address/NULL, no ambient name lookup.
 * context is unused. / Заимствовать selected plan/metadata; свой provider address/NULL, без ambient lookup,
 * context не используется.
 */
static DVM_LaunchAddress *discover(void *context, const DVM_ResourcePlan *selected, const FVM_FileInfo *metadata)
{
    (void)context;
    (void)metadata;
    return dvm_provider_launch(selected->provider, selected->relative);
}


/* EN/RU: See route.h; reuse all selection, cancellation and acknowledgement rules through one provider resolver.
 * См. route.h; повторно использовать selection/cancellation/ack через общий provider resolver.
 */
DVM_LaunchTicket *dvm_launch_route_discover(DVM_Namespace *group, const char *path, unsigned local_role,
    const DVM_LaunchSpec *spec)
{
    return dvm_launch_route_submit(group, path, local_role, spec, discover, NULL);
}


/* EN/RU: See route.h; the owned address replaces the temporary source plan, never retaining the caller namespace.
 * См. route.h; owned address заменяет временный source plan, не удерживая namespace caller.
 */
DVM_LaunchAddress *dvm_launch_route_resolve_source(DVM_Namespace *group, const char *path,
    unsigned local_role, DVM_Provider **selected)
{
    DVM_ResourcePlan source = {0};
    DVM_LaunchAddress *address = select_address(group, path, local_role, discover, NULL, &source);
    if (selected)
    {
        *selected = NULL;
        if (address && !dvm_provider_retain(source.provider))
        {
            dvm_launch_address_free(address);
            address = NULL;
        }
        if (address)
        {
            *selected = source.provider;
        }
    }
    dvm_resource_close(&source);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        if (selected)
        {
            dvm_provider_release(*selected);
            *selected = NULL;
        }
        dvm_launch_address_free(address);
        return NULL;
    }
    return address;
}



/* EN/RU: See route.h; compatibility entry returns only the address.
 * См. route.h; обычный вход возвращает только адрес. */
DVM_LaunchAddress *dvm_launch_route_resolve(DVM_Namespace *group, const char *path, unsigned local_role)
{
    return dvm_launch_route_resolve_source(group, path, local_role, NULL);
}
