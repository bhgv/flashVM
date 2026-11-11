/* Copyright (c) 2026 bhgv
 * EN: Managed-owner retirement queue and joined service shutdown.
 * RU: Очередь освобождения владельцев и завершение службы после join.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "gc/common/managed_queue.h"
#include "disvm/native_owner.h"
#include <stdio.h>
#include <stdlib.h>


#include "disvm/refcount.h"


/* [GC:] Only exact stopped collector may remove roots or run retained destroy.
 * Drop queue mutex before managed callbacks. The ticket's root/reference remain
 * owned through dequeue; accounting retires AFTER real destruction completes.
 * RU: На точном остановленном сборщике снять до budget владельцев; вернуть наличие оставшихся. Мьютекс очереди не удерживается при destroy. */
int dvm_managed_owner_drain(unsigned int budget)
{
    FVM_PortSync *mutex = dvm_managed_lock_service();
    DVM_NativeWorld *world;
    if (!mutex)
    {
        return 0;
    }
    world = dvm_managed_owners.world;
    fvm_port_sync_unlock(mutex);
    if (!world || !dvm_world_collector_owns(world))
    {
        dvm_managed_failure("retirement outside stopped collector");
    }
    while (budget--)
    {
        ManagedTicket *ticket;
        void *value;
        mutex = dvm_managed_lock_service();
        ticket = dvm_managed_owners.head;
        if (!ticket)
        {
            fvm_port_sync_unlock(mutex);
            break;
        }
        dvm_managed_owners.head = ticket->next;
        if (!dvm_managed_owners.head)
        {
            dvm_managed_owners.tail = NULL;
        }
        dvm_managed_owners.pending--;
        fvm_port_sync_unlock(mutex);
        value = ticket->value;
        if (!dvm_guard_registry_release(&ticket->access))
        {
            dvm_managed_failure("retiring ticket has a live sidecar borrower");
        }
        ptrdel(D2H(value));
        destroy(value);
        free(ticket);
        mutex = dvm_managed_lock_service();
        dvm_managed_owners.tickets--;
        dvm_managed_owners.released++;
        fvm_port_sync_unlock(mutex);
    }
    return dvm_managed_owner_pending() != 0;
}


/* Native close preflight, used BEFORE irreversible world registration sealing.
 * No live aliases/producers means no later legitimate ticket wake can begin.
 * RU: Проверить отсутствие живых нативных алиасов и незавершённых производителей; корни ожидающих tickets остаются в очереди. */
int dvm_managed_owner_can_close(void)
{
    FVM_PortSync *mutex = dvm_managed_lock_service();
    int result = 1;
    if (mutex)
    {
        result = !dvm_managed_owners.live && !dvm_managed_owners.producers;
        fvm_port_sync_unlock(mutex);
    }
    return result;
}


/* [GC:] Close from ACTIVE admission or the IDLE startup controller. Pending rooted work
 * is still drained by Fast; no root/reference is discarded on refusal.
 * RU: Закрыть захват на ACTIVE либо IDLE startup-контексте. Живые алиасы или производители дают отказ без изменения состояния. */
int dvm_managed_owner_close(void)
{
    int active = dvm_managed_owner_admitted();
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    int idle = dvm_native_owner_statistics(owner, &state) && state.phase == DVM_HEAP_IDLE && !state.pending_progress;
    FVM_PortSync *mutex = dvm_managed_lock_service();
    if (!mutex)
    {
        return 0;
    }
    if ((!active && (!idle || dvm_managed_owners.controller != dvm_native_context_current() ||
                     dvm_native_owner_world(owner) != dvm_managed_owners.world)) ||
        dvm_managed_owners.live || dvm_managed_owners.producers)
    {
        fvm_port_sync_unlock(mutex);
        return 0;
    }
    dvm_managed_owners.accepting = 0;
    fvm_port_sync_unlock(mutex);
    return 1;
}


/* [GC:] Joined/quiescent finish. Do not reset borrowed worker/world pointers on
 * outstanding work. Mutex remains native process-lifetime synchronization.
 * RU: После завершения производителей и ожидания простоя проверить пустоту службы. При diagnostics вывести счётчики; при живых владениях вернуть ноль без сброса. */
int dvm_managed_owner_finish(int diagnostics)
{
    FVM_PortSync *mutex = dvm_managed_lock_service();
    int result;
    if (!mutex)
    {
        return 1;
    }
    if (dvm_managed_owners.live || dvm_managed_owners.pending || dvm_managed_owners.producers ||
        dvm_managed_owners.tickets || dvm_managed_owners.accepting)
    {
        fvm_port_sync_unlock(mutex);
        return 0;
    }
    if (diagnostics)
    {
        printf("MANAGEDSHARED transferred=%lu with_peer=%lu\n",
               dvm_managed_owners.shared_released,
               dvm_managed_owners.shared_with_peer);
        printf(
            "MANAGEDOWNERS captured=%lu released=%lu live=%lu pending=%lu tickets=%lu producers=%lu failed=%d rooted_until_fast=1\n",
            dvm_managed_owners.captured,
            dvm_managed_owners.released,
            dvm_managed_owners.live,
            dvm_managed_owners.pending,
            dvm_managed_owners.tickets,
            dvm_managed_owners.producers,
            dvm_managed_owners.failed);
    }
    if (dvm_managed_owners.cells && !dvm_cell_domain_destroy(&dvm_managed_owners.cells))
    {
        fvm_port_sync_unlock(mutex);
        return 0;
    }
    if (dvm_managed_owners.guards && !dvm_guard_registry_destroy(&dvm_managed_owners.guards))
    {
        fvm_port_sync_unlock(mutex);
        return 0;
    }
    result = !dvm_managed_owners.failed;
    dvm_managed_owners.world = NULL;
    dvm_managed_owners.workers = NULL;
    fvm_port_sync_unlock(mutex);
    return result;
}


/* [GC:] EN: Borrow queue head on Fast and copy world under native exclusion.
 * RU: На Fast получить голову и скопировать world под нативным мьютексом. */
ManagedTicket *dvm_managed_retirement_peek(DVM_NativeWorld **world)
{
    FVM_PortSync *mutex = dvm_managed_lock_service();
    *world = NULL;
    if (!mutex)
    {
        return NULL;
    }
    *world = dvm_managed_owners.world;
    ManagedTicket *ticket = dvm_managed_owners.head;
    fvm_port_sync_unlock(mutex);
    return ticket;
}


/* [GC:] EN: Complete exactly the consumer-owned head ticket; reject corruption.
 * RU: Завершить принадлежащую потребителю голову ticket; при нарушении отказать. */
void dvm_managed_retirement_complete(ManagedTicket *ticket, int peer)
{
    FVM_PortSync *mutex = dvm_managed_lock_service();
    if (!mutex || !ticket || dvm_managed_owners.head != ticket || !dvm_managed_owners.pending ||
        !dvm_managed_owners.tickets)
    {
        dvm_managed_failure("invalid shared retirement completion");
    }
    dvm_managed_owners.head = ticket->next;
    if (!dvm_managed_owners.head)
    {
        dvm_managed_owners.tail = NULL;
    }
    dvm_managed_owners.pending--;
    dvm_managed_owners.tickets--;
    dvm_managed_owners.released++;
    dvm_managed_owners.shared_released++;
    dvm_managed_owners.shared_with_peer += peer != 0;
    fvm_port_sync_unlock(mutex);
    free(ticket);
}
