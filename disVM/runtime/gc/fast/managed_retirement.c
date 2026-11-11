/* Copyright (c) 2026 bhgv
 * EN: Bounded managed-ticket handoff to RC without stopped-world ownership.
 * RU: Ограниченная передача managed tickets в RC без владения остановленной кучей.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "../common/managed_queue.h"
#include "../../runtime_rc.h"
#include "../../root_registry.h"
#include "disvm/native_owner.h"


/* [GC:] EN: On bound Fast context, hand off up to budget queued ticket owners.
 * Wait for their native-reader cutoff without parking guests. RC accepts the
 * existing strong reference and roots it if queued before the ticket root is
 * removed. A refused handoff leaves the ticket, root and sidecar unchanged.
 * Return progress or retry; zero permits the existing retained fallback.
 * The world and RC service outlive this call. Only Fast consumes the FIFO.
 * RU: На привязанном Fast context передать до budget владельцев из очереди.
 * Дождаться эпохи читателей без остановки гостей. RC принимает сильную ссылку
 * и при постановке в очередь создаёт корень до снятия корня ticket. При отказе
 * ticket, корень и sidecar не меняются. Вернуть работу/повтор; ноль допускает
 * прежний резервный путь. Мир и RC живут весь вызов; очередь читает только Fast.
 */
unsigned int dvm_managed_owner_drain_shared(DVM_NativeContext *context, size_t budget)
{
    DVM_NativeWorld *world;
    if (!budget || context != dvm_native_context_current())
    {
        return 0;
    }
    ManagedTicket *ticket = dvm_managed_retirement_peek(&world);
    if (!ticket || !world || !ticket->epoch)
    {
        return 0;
    }
    DVM_HeapOwner *owner = dvm_native_owner_current();
    if (!dvm_native_owner_attach_service(owner, world))
    {
        return 0;
    }
    unsigned int work = 0;
    if (dvm_native_owner_try_enter_idle_shared(owner))
    {
        while (budget--)
        {
            DVM_NativeWorld *identity;
            ticket = dvm_managed_retirement_peek(&identity);
            if (!ticket || identity != world || !ticket->epoch)
            {
                break;
            }
            if (!dvm_world_epoch_elapsed(world, ticket->epoch))
            {
                work = work ? work : 1;
                break;
            }
            Heap *heap = D2H(ticket->value);
            if (!dvm_runtime_rc_defer(ticket->value))
            {
                break;
            }
            if (!dvm_guard_registry_release(&ticket->access))
            {
                dvm_managed_failure("shared ticket has a live sidecar borrower");
            }
            if (!dvm_runtime_root_remove(heap))
            {
                dvm_managed_failure("shared ticket lost its root");
            }
            dvm_managed_retirement_complete(ticket, dvm_world_has_active_guest(world));
            work++;
        }
        if (!dvm_native_owner_leave_idle_shared(owner))
        {
            dvm_managed_failure("shared ticket admission leave");
        }
    }
    if (!dvm_native_owner_detach(owner))
    {
        dvm_managed_failure("shared ticket admission detach");
    }
    return work;
}
