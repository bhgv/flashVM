/* Copyright (c) 2026 bhgv
 * EN: Native owner checks a copied required interface before environment preparation can begin.
 * RU: Native owner проверяет copied required interface до начала подготовки окружения.
 */
#include "ticket_internal.h"


/* EN/RU: See ticket.h; shared native requirements are immutable through the caller's request pin.
 * См. ticket.h; shared native requirements immutable при request pin caller.
 */
DVM_LaunchContract *dvm_launch_request_contract(DVM_LaunchRequest *request)
{
    return request ? dvm_launch_contract_clone(request->shared->contract) : NULL;
}


/* EN/RU: See ticket.h; contract immutable while request pinned, matching occurs outside request lock.
 * См. ticket.h; contract immutable при pinned request, matching вне request lock.
 */
int dvm_launch_request_check(DVM_LaunchRequest *request, const DVM_LaunchContract *exports)
{
    if (!request || !exports || dvm_cancel_requested(dvm_cancel_current()))
    {
        return 0;
    }
    LaunchShared *shared = request->shared;
    if (shared->contract && !dvm_launch_contract_matches(shared->contract, exports))
    {
        return 0;
    }
    fvm_port_sync_lock(shared->lock);
    int accepted = shared->state == DVM_LAUNCH_PENDING && !dvm_cancel_requested(shared->cancel) &&
        !dvm_cancel_requested(dvm_cancel_current());
    if (accepted)
    {
        shared->contract_checked = 1;
    }
    fvm_port_sync_unlock(shared->lock);
    return accepted;
}
