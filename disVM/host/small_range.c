/* Copyright (c) 2026 bhgv
 *
 * EN: Guard short native spans used by shared instructions.
 * RU: Блокировки коротких нативных диапазонов для параллельных инструкций.
 */
/* Compact three-small-span exclusion in the canonical cell/range domain. */
#include "disvm/small_range.h"
#include "cell_access_private.h"
#include "disvm/refcount.h"
#include <stdint.h>
#include <stdlib.h>

#define STRIPES DVM_CELL_STRIPES /* Existing canonical domain, no new namespace. */


/* Roll back partial/complete access only on exact current owner. No callbacks.
 * Creator still owns domain, so dropping the access borrow must be nonfinal. */
static void unwind(DVM_TripleAccess *access)
{
    while (access->held)
    {
        access->held--;
        if (!dvm_cell_guard_leave(access->domain, access->guards[access->held], access->context))
        {
            abort();
        }
    }
    while (access->count)
    {
        access->guards[--access->count] = NULL;
    }
    if (dvm_refcount_release_u(&access->domain->references) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
    access->domain = NULL;
    access->context = NULL;
}


/* Verify private zeroed access before acquiring any native resource. */
static int empty(DVM_TripleAccess *access)
{
    unsigned int index;
    if (!access || access->domain || access->context || access->count || access->held)
    {
        return 0;
    }
    for (index = 0; index < 6; index++)
    {
        if (access->guards[index])
        {
            return 0;
        }
    }
    return 1;
}


/* Compute both canonical boundary stripes without touching operand storage.
 * Address geometry is not mapping/provenance/root validation. */
static int range(const void *slot, size_t length, unsigned int *keys)
{
    uintptr_t address = (uintptr_t)slot;
    uintptr_t width = sizeof(void *);
    uintptr_t last;
    if (!length || length > width || address < width ||
        address > (uintptr_t)-1 - (length - 1))
    {
        return 0;
    }
    last = address + length - 1;
    keys[0] = (unsigned int)((address / width) % STRIPES);
    keys[1] = (unsigned int)((last / width) % STRIPES);
    return 1;
}


/* [PERF SMALL-097] Six-key selection avoids generic 256-entry metadata scans.
 * Workload timing is in runtime/PERFORMANCE-COSTS.md, not per-lock attribution. */


/* domain is independently live; source/second/destination are borrowed small
 * spans and access is zero owner-local storage. Return ACQUIRED/BUSY/REFUSED.
 * Claim unique stripes in ascending index order. A failed claim drops all held
 * guards before returning; callers may retry only before ownership effects. */
DVM_CellResult dvm_small_range_access_begin(DVM_CellDomain *domain,
    const void *source, size_t source_size, const void *second, size_t second_size,
    void *destination, size_t destination_size,
    DVM_TripleAccess *access)
{
    DVM_NativeContext *context = dvm_native_context_current();
    unsigned int keys[6];
    unsigned int index;
    unsigned int other;
    if (!domain || !context || dvm_native_context_object_count(context) ||
        !empty(access) || !range(source, source_size, keys) ||
        !range(second, second_size, keys + 2) ||
        !range(destination, destination_size, keys + 4))
    {
        return DVM_CELL_REFUSED;
    }
    for (index = 0; index < 6; index++)
    {
        for (other = index + 1; other < 6; other++)
        {
            if (keys[other] < keys[index])
            {
                unsigned int temporary = keys[index];
                keys[index] = keys[other];
                keys[other] = temporary;
            }
        }
    }
    if (dvm_refcount_retain_u(&domain->references) != DVM_REFCOUNT_LIVE)
    {
        return DVM_CELL_REFUSED;
    }
    access->domain = domain;
    access->context = context;
    for (index = 0; index < 6; index++)
    {
        if (index && keys[index] == keys[index - 1])
        {
            continue;
        }
        access->guards[access->count++] = &domain->stripes[keys[index]];
    }
    for (index = 0; index < access->count; index++)
    {
        DVM_ObjectResult result = domain->blocking ? dvm_object_guard_enter(access->guards[index]) :
            dvm_object_guard_try_enter(access->guards[index]);
        if (result != DVM_OBJECT_ACQUIRED)
        {
            unwind(access);
            return result == DVM_OBJECT_BUSY ? DVM_CELL_BUSY : DVM_CELL_REFUSED;
        }
        access->held++;
    }
    if (dvm_cell_write_reserved(domain, destination, destination_size) ||
        dvm_cell_read_reserved(domain, source, source_size) || dvm_cell_read_reserved(domain, second, second_size))
    {
        unwind(access);
        return DVM_CELL_BUSY;
    }
    return DVM_CELL_ACQUIRED;
}


/* Restore the previous prefix after an extension refuses. access is exact-owner
 * private metadata; original is its prior count. No domain borrow is consumed.
 * Guard release can use native wake metadata, never a payload/VM callback. */
static void undo_extension(DVM_TripleAccess *access, unsigned int original)
{
    while (access->held > original)
    {
        access->held--;
        if (!dvm_cell_guard_leave(access->domain, access->guards[access->held], access->context))
        {
            abort();
        }
    }
    while (access->count > original)
    {
        access->guards[--access->count] = NULL;
    }
}


/* [PERF ARRAYLEN-102] Two-key extension cost is not isolated from the full
 * runtime profile; see runtime/PERFORMANCE-COSTS.md.
 * access owns an existing canonical-domain borrow; span/size are borrowed live
 * storage, not a payload capability. New keys use bounded try-only claims with
 * no release gap in the original prefix. Failed additions roll back alone;
 * caller must end the still-live original access before any VM/error/wait. */
DVM_CellResult dvm_small_range_access_extend(DVM_TripleAccess *access,
    const void *span, size_t size)
{
    DVM_NativeContext *context = dvm_native_context_current();
    unsigned int keys[2];
    unsigned int original;
    unsigned int index;
    unsigned int other;
    if (!context || !access || !access->domain || access->context != context ||
        !access->count || access->count > 6 || access->held != access->count ||
        dvm_native_context_object_count(context) != access->held || !range(span, size, keys))
    {
        return DVM_CELL_REFUSED;
    }
    if (keys[1] < keys[0])
    {
        unsigned int temporary = keys[0];
        keys[0] = keys[1];
        keys[1] = temporary;
    }
    original = access->count;
    for (index = 0; index < 2; index++)
    {
        DVM_ObjectGuard *guard = &access->domain->stripes[keys[index]];
        for (other = 0; other < access->count; other++)
        {
            if (access->guards[other] == guard)
            {
                break;
            }
        }
        if (other != access->count)
        {
            continue;
        }
        if (access->count == 6)
        {
            undo_extension(access, original);
            return DVM_CELL_REFUSED;
        }
        access->guards[access->count++] = guard;
    }
    for (index = original; index < access->count; index++)
    {
        DVM_ObjectResult result = dvm_object_guard_try_enter(access->guards[index]);
        if (result != DVM_OBJECT_ACQUIRED)
        {
            undo_extension(access, original);
            return result == DVM_OBJECT_BUSY ? DVM_CELL_BUSY : DVM_CELL_REFUSED;
        }
        access->held++;
    }
    if (dvm_cell_read_reserved(access->domain, span, size))
    {
        undo_extension(access, original);
        return DVM_CELL_BUSY;
    }
    return DVM_CELL_ACQUIRED;
}


/* Foreign/depth refusal precedes mutation; owner may retry after extra unlock. */
int dvm_small_range_access_end(DVM_TripleAccess *access)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!context || !access || access->context != context || !access->count ||
        access->held != access->count ||
        dvm_native_context_object_count(context) != access->held)
    {
        return 0;
    }
    unwind(access);
    return 1;
}


