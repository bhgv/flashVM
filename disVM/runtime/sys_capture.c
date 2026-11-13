/* Copyright (c) 2026 bhgv
 * EN: Capture Sys frames and inspect live owners with atomic native counts.
 * RU: Захват Sys frames и проверка живых owners с атомарными native counts.
 */


/* Adapt Inferno's captured Sys boundaries to the independent NC64 port host.
 * Origin: Inferno-OS-bhgv experiment 86a2bf2, sysstatusframe/sysresultowner guards.
 * Keep OS I/O in FlashVM ports and keep matched generated frame sizes explicit. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "sys_capture.h"
#include "disvm/refcount.h"
#include <stdint.h>
#include <string.h>


/* EN: Copy admitted frame into local aligned storage; size/ret_offset identify
 * the generated return slot, result_bytes its whole span. Borrowed original roots
 * stay live; invalid bounds raise before callbacks. See sys_capture.h.
 * RU: Копировать frame в local storage; size/ret_offset задают generated ret slot,
 * result_bytes — полный span. Original roots живы; invalid bounds дают error. */
void dvm_sys_capture(const void *frame, void *local, size_t size, size_t ret_offset, size_t result_bytes)
{
    void *result;
    if (!frame || frame == H)
    {
        error(exNilref);
    }
    if (!local || ret_offset > size || sizeof(result) > size - ret_offset || (uintptr_t)frame > UINTPTR_MAX - size)
    {
        error(exBounds);
    }
    memmove(local, frame, size);
    memmove(&result, (const unsigned char *)local + ret_offset, sizeof(result));
    if (!result || result == H)
    {
        error(exNilref);
    }
    if (!result_bytes || (uintptr_t)result > UINTPTR_MAX - result_bytes)
    {
        error(exBounds);
    }
}


/* [GC:] EN: Borrow a rooted slot's owner, using an atomic count after validating
 * heap alignment. slot/value lifetime belongs to caller; this is not a retain.
 * RU: Заимствовать owner rooted slot, читать count атомарно после проверки heap
 * alignment. Slot/value живут у caller; новая ссылка не приобретается. */
void *dvm_sys_owner(const void *slot)
{
    void *owner;
    uintptr_t address;
    ulong references;
    memmove(&owner, slot, sizeof(owner));
    if (!owner)
    {
        error(exNilref);
    }
    if (owner != H)
    {
        address = (uintptr_t)owner;
        if (address <= sizeof(Heap) || (address - sizeof(Heap)) % sizeof(void *))
        {
            error(exHeap);
        }
        references = dvm_refcount_load_u(&D2H(owner)->ref);
        if (!references)
        {
            error(exHeap);
        }
    }
    return owner;
}


/* [GC:] EN: Validate count captured owners, including aliases, before destruction.
 * Inputs passed dvm_sys_owner and stay independently owned. Atomic count samples
 * do not replace that lifetime or authorize arbitrary shared payload access.
 * RU: Проверить count owners с aliases до destruction. Inputs прошли dvm_sys_owner
 * и остаются во владении caller. Atomic count не заменяет lifetime/payload guards. */
void dvm_sys_owners(void *const *owners, size_t count)
{
    size_t index;
    if (!owners || !count || count > DVM_SYS_MAX_OWNERS)
    {
        error(exBounds);
    }
    for (index = 0; index < count; index++)
    {
        size_t other;
        ulong references;
        size_t copies = 0;
        if (owners[index] == H)
        {
            continue;
        }
        for (other = 0; other < count; other++)
        {
            if (owners[other] == owners[index])
            {
                copies++;
            }
        }
        references = dvm_refcount_load_u(&D2H(owners[index])->ref);
        if (references < copies)
        {
            error(exHeap);
        }
    }
}
