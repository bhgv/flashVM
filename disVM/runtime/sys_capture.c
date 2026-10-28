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
#include <stdint.h>
#include <string.h>


/* Copy an admitted generated frame and validate its complete original result span.
 * See sys_capture.h for rooting, lifetime, error and trusted-storage restrictions. */
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


/* Borrow a mapped slot's previous owner without assuming VM slot alignment. */
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
        memmove(&references, (const unsigned char *)owner - sizeof(Heap) + offsetof(Heap, ref), sizeof(references));
        if (!references)
        {
            error(exHeap);
        }
    }
    return owner;
}


/* Validate alias multiplicities before any captured result owner is destroyed. */
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
        memmove(
            &references, (const unsigned char *)owners[index] - sizeof(Heap) + offsetof(Heap, ref), sizeof(references));
        if (references < copies)
        {
            error(exHeap);
        }
    }
}
