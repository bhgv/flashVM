/* Copyright (c) 2026 bhgv
 *
 * EN: Private stripe storage and optional GC observation metadata.
 * RU: Внутреннее хранение блокировок полей и метаданные наблюдения GC.
 */
/* Shared private domain geometry; never a serialized/public Heap layout. */
#ifndef DVM_CELL_ACCESS_PRIVATE_H
#define DVM_CELL_ACCESS_PRIVATE_H
#include "disvm/cell_access.h"
#include <limits.h>
#include <stdint.h>

#define DVM_CELL_STRIPES 256 /* Collision exclusion, not an object/thread quota. */


/* Creator and each complete/partial native access hold independent borrows. */
struct DVM_CellDomain
{
    unsigned long references; /* Checked native count; caller protects first lookup. */
    int blocking; /* Initial claims wait in stripe order when enabled at startup. */
    DVM_ObjectDomain *parking; /* Owned shared parking metadata. */
    unsigned int reservation_active; /* Atomic phase: 0 inactive, 1 writes, 2 reads and writes. */
    const uintptr_t *reserved_cells; /* Sorted exact addresses under the registry mutex. */
    DVM_NativeContext *reservation_owner; /* Collector allowed to drain reserved fields. */
    size_t reserved_count; /* Size of reserved_cells under the registry mutex. */
    DVM_NativeContext *watch_owner; /* Atomic observer, NULL outside a watched pass. */
    unsigned long revision[DVM_CELL_STRIPES]; /* Guarded by the corresponding stripe. */
    DVM_ObjectGuard stripes[DVM_CELL_STRIPES]; /* Stable native guard addresses. */
};
/* EN: Check a guarded source span during the exact-field claim phase.
 * RU: Проверить защищённый диапазон чтения во время захвата счётчиков. */
int dvm_cell_read_reserved(DVM_CellDomain *domain, const void *source, size_t size);
int dvm_cell_write_reserved(DVM_CellDomain *domain, const void *destination, size_t size);


/* EN: Release guard in domain on context and invalidate a foreign observer.
 * RU: Освободить guard в domain на context и уведомить чужого наблюдателя. */
static inline int dvm_cell_guard_leave(DVM_CellDomain *domain, DVM_ObjectGuard *guard,
    DVM_NativeContext *context)
{
    DVM_NativeContext *observer = __atomic_load_n(&domain->watch_owner, __ATOMIC_ACQUIRE);
    if (observer && observer != context)
    {
        unsigned long *revision = &domain->revision[guard - domain->stripes];
        if (*revision != ULONG_MAX)
        {
            (*revision)++;
        }
    }
    return dvm_object_guard_leave(guard);
}
#endif
