/* Copyright (c) 2026 bhgv
 * EN: Managed ticket guard identity and role-checked cell/range access.
 * RU: Идентичность блокировок tickets и проверка роли доступа к ячейкам/диапазонам.
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


/* [GC:] Borrow one canonical sidecar from an already rooted ticket. entry is
 * current-owner-local and linked; exact ACTIVE world is checked before access.
 * Keep entry/key alive and obtain all guards BEFORE locking any of them. This
 * adds no payload access permission; legacy world exclusion still applies.
 * RU: Заимствовать каноническую блокировку живого entry текущего ACTIVE-владельца. Entry и его ticket должны жить до завершения работы с guard. */
DVM_ObjectGuard *dvm_managed_owner_guard(const DVM_OwnerEntry *entry)
{
    ManagedTicket *ticket;
    if (!dvm_managed_owner_admitted() || !entry || !entry->scope ||
        entry->scope->context != dvm_native_context_current() || entry->ops != &dvm_managed_ticket_ops ||
        !entry->object)
    {
        return NULL;
    }
    ticket = entry->object;
    if (ticket->world != dvm_native_owner_world(dvm_native_owner_current()))
    {
        return NULL;
    }
    return dvm_guard_registry_guard(ticket->access);
}


/* Native metadata only. Holding service sync prevents finish from destroying
 * the registry while its own mutex supplies the count snapshot. No VM callback,
 * root/payload access or persistent node lifetime permission is acquired.
 * RU: Получить число канонических sidecars под мьютексами службы и реестра; владение объектами не приобретается. */
unsigned long dvm_managed_owner_guard_entries(void)
{
    FVM_PortSync *mutex = dvm_managed_lock_service();
    unsigned long result = 0;
    if (mutex)
    {
        result = dvm_guard_registry_entries(dvm_managed_owners.guards);
        fvm_port_sync_unlock(mutex);
    }
    return result;
}


/* Borrow the live native cell domain before the caller suspends its guest.
 * The runtime and its registered guest must outlive all use of this pointer.
 * RU: Получить домен ячеек текущего ACTIVE-владельца либо NULL; зарегистрированный исполнитель удерживает службу весь период использования. */
DVM_CellDomain *dvm_managed_cell_domain(void)
{
    return dvm_managed_owner_admitted() ? dvm_managed_owners.cells : NULL;
}


/* EN: Borrow the cell lock domain while a SHARED dispatch pins its service.
 * RU: Получить домен блокировок, пока SHARED удерживает сервис. */
DVM_CellDomain *dvm_managed_dispatch_cell_domain(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    if (!dvm_native_owner_statistics(owner, &state) || state.phase != DVM_HEAP_SHARED || state.pending_progress ||
        dvm_native_context_object_count(dvm_native_context_current()))
    {
        return NULL;
    }
    FVM_PortSync *mutex = dvm_managed_lock_service();
    if (!mutex)
    {
        return NULL;
    }
    DVM_CellDomain *domain =
        dvm_managed_owners.world == dvm_native_owner_world(owner) ? dvm_managed_owners.cells : NULL;
    fvm_port_sync_unlock(mutex);
    return domain;
}


/* [GC:] Stable service/world identity is checked before native registry use.
 * The registered owner stabilizes the service and caller-owned spans.
 * Cell guards do not replace source lifetime, roots or future GC epoch proof.
 * RU: Захватить source/destination в access после проверки ACTIVE-роли. Диапазоны живут независимо; результат не создаёт корней или сильных ссылок. */
DVM_CellResult dvm_managed_cells_begin(const void *source, void *destination, DVM_CellAccess *access)
{
    if (!dvm_managed_owner_admitted())
    {
        return DVM_CELL_REFUSED;
    }
    return dvm_cell_access_begin(dvm_managed_owners.cells, source, destination, access);
}


/* [GC:] Service identity and ACTIVE exclusion stabilize domain/span metadata.
 * No ownership/root permission follows; native small-span begin never allocates.
 * RU: Захватить живые source/source_size и destination/destination_size в access на текущем ACTIVE-контексте; при неверной роли вернуть REFUSED. */
DVM_CellResult dvm_managed_spans_begin(
    const void *source, size_t source_size, void *destination, size_t destination_size, DVM_CellAccess *access)
{
    if (!dvm_managed_owner_admitted())
    {
        return DVM_CELL_REFUSED;
    }
    return dvm_cell_access_spans_begin(
        dvm_managed_owners.cells, source, source_size, destination, destination_size, access);
}


/* [GC:] Same role/lifetime contract for wide spans, sharing the SAME domain.
 * Size geometry is not a managed root, provenance check or payload capability.
 * RU: Захватить общие диапазоны source/source_size и destination/destination_size в access. Время жизни памяти обеспечивает вызывающий. */
DVM_CellResult dvm_managed_ranges_begin(
    const void *source, size_t source_size, void *destination, size_t destination_size, DVM_RangeAccess *access)
{
    if (!dvm_managed_owner_admitted())
    {
        return DVM_CELL_REFUSED;
    }
    return dvm_range_access_begin(dvm_managed_owners.cells, source, source_size, destination, destination_size, access);
}


/* [GC:] first/second/output spans and access follow range_access_three_begin.
 * Admit exact current service owner before any domain lookup or payload access.
 * RU: Защитить first/first_size, second/second_size и output/output_size в access после проверки роли. Отказы и откат определены range_access_three_begin. */
DVM_CellResult dvm_managed_three_begin(const void *first,
                                       size_t first_size,
                                       const void *second,
                                       size_t second_size,
                                       void *output,
                                       size_t output_size,
                                       DVM_RangeAccess *access)
{
    if (!dvm_managed_owner_admitted())
    {
        return DVM_CELL_REFUSED;
    }
    return dvm_range_access_three_begin(
        dvm_managed_owners.cells, first, first_size, second, second_size, output, output_size, access);
}


/* [GC:] first/second/output borrow live small spans; access is zero owner-local
 * storage. Preflight exact ACTIVE service before canonical domain lookup.
 * RU: Захватить три малых живых диапазона first/first_size, second/second_size, output/output_size в нулевой access. При неверной роли отказать до чтения памяти. */
DVM_CellResult dvm_managed_small_three_begin(const void *first,
                                             size_t first_size,
                                             const void *second,
                                             size_t second_size,
                                             void *output,
                                             size_t output_size,
                                             DVM_TripleAccess *access)
{
    if (!dvm_managed_owner_admitted())
    {
        return DVM_CELL_REFUSED;
    }
    return dvm_small_range_access_begin(
        dvm_managed_owners.cells, first, first_size, second, second_size, output, output_size, access);
}
