/* Copyright (c) 2026 bhgv
 * EN: Blocking native owner admission with independent queue/cancellation wake state.
 * RU: Блокирующий native admission владельца с независимым queue/cancellation wake state.
 */
#include "service_internal.h"
#include <stdlib.h>


/* EN/RU: Native-only callback recipient, independently pinned through detached callback retirement.
 * Native-only получатель callbacks, отдельно удерживаемый до завершения detached callbacks.
 */
typedef struct LaunchWait
{
    FVM_PortSync *lock; /* Protects references, readiness and sticky cancellation; never service/VM state. */
    unsigned references; /* Waiting caller plus at most one journal tap and one cancellation tap. */
    int changed; /* Coalesced wake hint; queue inspection remains authoritative. */
    int cancelled; /* Sticky cancellation for this wait only, never service close. */
} LaunchWait;


/* EN/RU: Consume one independent native wait-state pin; final free occurs outside lock, without callbacks/I/O.
 * Поглотить native pin wait-state; final free вне lock, без callbacks/I/O.
 */
static void release(void *state)
{
    LaunchWait *wait = state;
    fvm_port_sync_lock(wait->lock);
    int last = --wait->references == 0;
    fvm_port_sync_unlock(wait->lock);
    if (last)
    {
        fvm_port_sync_destroy(wait->lock);
        free(wait);
    }
}


/* EN/RU: Reserve one callback pin under the caller's live pin; at most three owners, no allocation/I/O.
 * Зарезервировать callback pin под live pin caller; не более трёх owners, без allocation/I/O.
 */
static void retain(LaunchWait *wait)
{
    fvm_port_sync_lock(wait->lock);
    wait->references++;
    fvm_port_sync_unlock(wait->lock);
}


/* EN/RU: Copy queue hint into independent native state and wake native waiters; no service/guest pointer or execution.
 * Скопировать queue hint в independent native state, разбудить native waiters; без service/guest pointers/execution.
 */
static void changed(void *state, const DVM_ChangeEvent *event)
{
    LaunchWait *wait = state;
    (void)event;
    fvm_port_sync_lock(wait->lock);
    wait->changed = 1;
    fvm_port_sync_signal_all(wait->lock);
    fvm_port_sync_unlock(wait->lock);
}


/* EN/RU: Cancel only this independent waiting operation, not queued requests or other handlers; no service/VM access.
 * Отменить лишь independent ожидание, не queued requests/другие handlers; без service/VM access.
 */
static void cancelled(void *state)
{
    LaunchWait *wait = state;
    fvm_port_sync_lock(wait->lock);
    wait->cancelled = 1;
    fvm_port_sync_signal_all(wait->lock);
    fvm_port_sync_unlock(wait->lock);
}


/* EN/RU: Borrow owner/wake state through blocking native wait; return exactly one owned review through out on success.
 * Clear hint before inspecting queue so publication before/during sleep cannot be lost. No provider I/O or guest execution.
 * Заимствовать owner/wake state до конца native wait; при success выдать ровно один owned review через out.
 * Hint очищается до queue inspection: publication перед/во время sleep не теряется. Без provider I/O/guest execution.
 */
static DVM_LaunchWaitResult receive(DVM_LaunchService *owner, LaunchWait *wait, DVM_LaunchReview **out)
{
    for (;;)
    {
        fvm_port_sync_lock(wait->lock);
        int stop = wait->cancelled;
        wait->changed = 0;
        fvm_port_sync_unlock(wait->lock);
        if (stop)
        {
            return DVM_LAUNCH_WAIT_CANCELLED;
        }
        DVM_LaunchReview *review = dvm_launch_service_next(owner);
        if (review)
        {
            *out = review; /* Acknowledged queue claim wins a concurrent later cancellation. */
            return DVM_LAUNCH_WAIT_REQUEST;
        }
        DVM_LaunchServiceInfo info;
        if (!dvm_launch_service_info(owner, &info))
        {
            return DVM_LAUNCH_WAIT_FAILED;
        }
        if (info.closed)
        {
            return DVM_LAUNCH_WAIT_CLOSED;
        }
        fvm_port_sync_lock(wait->lock);
        while (!wait->changed && !wait->cancelled)
        {
            fvm_port_sync_wait(wait->lock);
        }
        fvm_port_sync_unlock(wait->lock);
    }
}


/* EN/RU: See service.h; detach without waiting for already admitted native callbacks, which own independent state.
 * См. service.h; detach не ждёт already admitted native callbacks: они владеют independent state.
 */
DVM_LaunchWaitResult dvm_launch_service_wait(DVM_LaunchService *owner, DVM_LaunchReview **out)
{
    if (out)
    {
        *out = NULL;
    }
    if (!owner || !out)
    {
        return DVM_LAUNCH_WAIT_FAILED;
    }
    DVM_Cancel *source = dvm_cancel_current(); /* Borrowed through current native scope, not inherited by other threads. */
    if (dvm_cancel_requested(source))
    {
        return DVM_LAUNCH_WAIT_CANCELLED;
    }
    LaunchWait *wait = calloc(1, sizeof(*wait)); /* Never contains service/request/provider/guest addresses. */
    if (!wait)
    {
        return DVM_LAUNCH_WAIT_FAILED;
    }
    wait->lock = fvm_port_sync_create();
    if (!wait->lock)
    {
        free(wait);
        return DVM_LAUNCH_WAIT_FAILED;
    }
    wait->references = 1;
    retain(wait);
    DVM_NoticeTap *notice = dvm_notice_subscribe(dvm_changes_notices(owner->shared->changes), changed, wait, release);
    if (!notice)
    {
        release(wait); /* Failed subscribe leaves reserved state ownership with this caller. */
        release(wait);
        return DVM_LAUNCH_WAIT_FAILED;
    }
    DVM_CancelTap *cancel = NULL; /* Optional per-operation scope subscription, never service-wide cancellation. */
    if (source)
    {
        retain(wait);
        cancel = dvm_cancel_subscribe(source, cancelled, wait, release);
        if (!cancel)
        {
            release(wait);
        }
    }
    DVM_LaunchWaitResult result = source && !cancel ? DVM_LAUNCH_WAIT_FAILED : receive(owner, wait, out);
    dvm_cancel_unsubscribe(cancel);
    dvm_notice_unsubscribe(notice);
    release(wait);
    return result;
}
