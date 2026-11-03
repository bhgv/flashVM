/* Arbitrary byte ranges use the SAME native stripe identities as pointer cells. */
#include "disvm/range_access.h"
#include "cell_access_private.h"
#include "disvm/refcount.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if DVM_RANGE_GUARDS != DVM_CELL_STRIPES
#error Native range capacity must match the shared stripe domain
#endif


/* Zero-before-begin check is owner-local and does not dereference operand data. */
static int empty(DVM_RangeAccess *access)
{
    unsigned int index;
    if (!access || access->domain || access->context || access->count || access->held)
    {
        return 0;
    }
    for (index = 0; index < DVM_RANGE_GUARDS; index++)
    {
        if (access->guards[index])
        {
            return 0;
        }
    }
    return 1;
}


/* Add address-range stripes to private bitset. No mapped storage is touched.
 * Contiguous spans covering a stripe period select all guards without looping
 * over unbounded input length. Empty spans ignore even NULL/wrapped addresses. */
static int select_span(const void *source, size_t length, unsigned char *bits)
{
    uintptr_t address = (uintptr_t)source;
    uintptr_t first;
    uintptr_t last;
    uintptr_t cells;
    uintptr_t index;
    if (!length)
    {
        return 1;
    }
    if (address < sizeof(void *) || length > (uintptr_t)-1 - address)
    {
        return 0;
    }
    first = address / sizeof(void *);
    last = (address + length - 1) / sizeof(void *);
    cells = last - first + 1;
    if (cells >= DVM_CELL_STRIPES)
    {
        memset(bits, 255, DVM_CELL_STRIPES / 8);
        return 1;
    }
    for (index = 0; index < cells; index++)
    {
        unsigned int stripe = (unsigned int)((first + index) % DVM_CELL_STRIPES);
        bits[stripe / 8] |= (unsigned char)(1U << (stripe % 8));
    }
    return 1;
}


/* Release exact owner prefix and native borrow; invariant failure cannot resume
 * a VM/error callback under leaked guards. Creator owns one surviving borrow. */
static void unwind(DVM_RangeAccess *access)
{
    while (access->held)
    {
        access->held--;
        if (!dvm_object_guard_leave(access->guards[access->held]))
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


/* Geometry and full selection precede claims; all partial failures unwind.
 * Guard order is stripe order, independent of source/destination argument order. */
DVM_CellResult dvm_range_access_begin(DVM_CellDomain *domain,
    const void *source, size_t source_size, void *destination, size_t destination_size,
    DVM_RangeAccess *access)
{
    unsigned char bits[DVM_CELL_STRIPES / 8] = {0};
    DVM_NativeContext *context = dvm_native_context_current();
    unsigned int index;
    if (!domain || !context || dvm_native_context_object_count(context) || !empty(access) ||
        !select_span(source, source_size, bits) || !select_span(destination, destination_size, bits))
    {
        return DVM_CELL_REFUSED;
    }
    if (dvm_refcount_retain_u(&domain->references) != DVM_REFCOUNT_LIVE)
    {
        return DVM_CELL_REFUSED;
    }
    access->domain = domain;
    access->context = context;
    for (index = 0; index < DVM_CELL_STRIPES; index++)
    {
        if (bits[index / 8] & (1U << (index % 8)))
        {
            access->guards[access->count++] = &domain->stripes[index];
        }
    }
    for (index = 0; index < access->count; index++)
    {
        DVM_ObjectResult result = dvm_object_guard_try_enter(access->guards[index]);
        if (result != DVM_OBJECT_ACQUIRED)
        {
            unwind(access);
            return result == DVM_OBJECT_BUSY ? DVM_CELL_BUSY : DVM_CELL_REFUSED;
        }
        access->held++;
    }
    return DVM_CELL_ACQUIRED;
}


/* Zero-count success still owns a borrow, distinguishable from zero storage. */
int dvm_range_access_end(DVM_RangeAccess *access)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!context || !access || !access->domain || access->context != context ||
        access->held != access->count || dvm_native_context_object_count(context) != access->held)
    {
        return 0;
    }
    unwind(access);
    return 1;
}
