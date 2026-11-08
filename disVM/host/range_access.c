/* Copyright (c) 2026 bhgv
 *
 * EN: Select and acquire native guards for byte ranges and pointer cells.
 * RU: Выбор и захват нативных блокировок для диапазонов байтов и полей ссылок.
 */
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


/* Acquire the selected stripes for domain in the current native context.
 * Access is caller-owned, empty storage. Failed claims release the acquired
 * prefix and domain reference before returning BUSY or REFUSED. */
static DVM_CellResult begin_selected(DVM_CellDomain *domain,
    const unsigned char *bits, DVM_RangeAccess *access)
{
    DVM_NativeContext *context = dvm_native_context_current();
    unsigned int index;
    if (!domain || !context || dvm_native_context_object_count(context) || !empty(access))
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


/* Select three borrowed spans before taking any guards. Domain and access
 * follow begin_selected's ownership and rollback contract. */
DVM_CellResult dvm_range_access_three_begin(DVM_CellDomain *domain,
    const void *source, size_t source_size, const void *second, size_t second_size,
    void *destination, size_t destination_size, DVM_RangeAccess *access)
{
    unsigned char bits[DVM_CELL_STRIPES / 8] = {0};
    if (!select_span(source, source_size, bits) || !select_span(second, second_size, bits) ||
        !select_span(destination, destination_size, bits))
    {
        return DVM_CELL_REFUSED;
    }
    return begin_selected(domain, bits, access);
}


/* EN: Compute plan for count pointer cells without reading their contents.
 * RU: Вычислить plan для count полей cells без чтения их содержимого. */
int dvm_range_plan_cells(DVM_RangePlan *plan, void *const *cells, size_t count)
{
    DVM_RangePlan candidate = {0};
    if (!plan || (count && !cells))
    {
        return 0;
    }
    for (size_t index = 0; index < count; index++)
    {
        if (!select_span(cells[index], sizeof(void *), candidate.bits))
        {
            return 0;
        }
    }
    *plan = candidate;
    return 1;
}


/* EN: Try to acquire plan in domain. Borrowed access is empty on entry.
 * RU: Попробовать захватить plan в domain. На входе access пуст. */
DVM_CellResult dvm_range_plan_begin(DVM_CellDomain *domain,
    const DVM_RangePlan *plan, DVM_RangeAccess *access)
{
    return plan ? begin_selected(domain, plan->bits, access) : DVM_CELL_REFUSED;
}


/* EN: Build and acquire the stripe union for count cells in domain.
 * RU: Построить и захватить набор блокировок count полей cells в domain через access. */
DVM_CellResult dvm_range_access_cells_begin(DVM_CellDomain *domain,
    void *const *cells, size_t count, DVM_RangeAccess *access)
{
    DVM_RangePlan plan;
    if (!dvm_range_plan_cells(&plan, cells, count))
    {
        return DVM_CELL_REFUSED;
    }
    return dvm_range_plan_begin(domain, &plan, access);
}


/* Search only the unchanged sorted original subset. access/guard are live
 * same-domain native metadata, count is its prior owned count. Return boolean;
 * no payload/VM access, allocation, reference operation or owner wait. */
static int selected_before(DVM_RangeAccess *access, unsigned int count, DVM_ObjectGuard *guard)
{
    unsigned int low = 0;
    unsigned int high = count;
    while (low < high)
    {
        unsigned int middle = low+(high-low)/2;
        if (access->guards[middle] == guard)
        {
            return 1;
        }
        if (access->guards[middle] < guard)
        {
            low = middle+1;
        }
        else
        {
            high = middle;
        }
    }
    return 0;
}


/* Drop only a newly appended acquisition prefix. original is prior count;
 * access stays exact-owner/live. No domain borrow or original guard is dropped. */
static void undo_additions(DVM_RangeAccess *access, unsigned int original)
{
    while (access->held > original)
    {
        access->held--;
        if (!dvm_object_guard_leave(access->guards[access->held]))
        {
            abort();
        }
    }
    while (access->count > original)
    {
        access->guards[--access->count] = NULL;
    }
}


/* Extend access by span/size under its existing creator-independent borrow.
 * Metadata stays private. Failure preserves original sorted metadata/claims;
 * success merges newly held keys without releasing any guard. No object-owner
 * park or allocation; native mutex fallback/wake contract remains unchanged. */
/* [PERF LISTLEN-103] Long walks retain all visited stripes; measured whole-profile
 * cost and collision/sorting candidates are tracked in runtime/PERFORMANCE-COSTS.md. */
DVM_CellResult dvm_range_access_extend(DVM_RangeAccess *access,
    const void *span, size_t size)
{
    DVM_NativeContext *context = dvm_native_context_current();
    unsigned int keys[DVM_RANGE_GUARDS];
    unsigned int key_count = 0;
    unsigned int original;
    unsigned int item;
    uintptr_t address = (uintptr_t)span;
    if (!context || !access || !access->domain || access->context != context ||
        access->count > DVM_RANGE_GUARDS || access->held != access->count ||
        dvm_native_context_object_count(context) != access->held)
    {
        return DVM_CELL_REFUSED;
    }
    if (!size)
    {
        return DVM_CELL_ACQUIRED;
    }
    if (address < sizeof(void *) || size > (uintptr_t)-1-address)
    {
        return DVM_CELL_REFUSED;
    }
    if (size <= sizeof(void *))
    {
        keys[0] = (unsigned int)((address/sizeof(void *))%DVM_CELL_STRIPES);
        keys[1] = (unsigned int)(((address+size-1)/sizeof(void *))%DVM_CELL_STRIPES);
        if (keys[1] < keys[0])
        {
            unsigned int temporary = keys[0];
            keys[0] = keys[1];
            keys[1] = temporary;
        }
        key_count = keys[0] == keys[1] ? 1 : 2;
    }
    else
    {
        unsigned char selected[DVM_CELL_STRIPES/8] = {0};
        unsigned int stripe;
        if (!select_span(span, size, selected))
        {
            return DVM_CELL_REFUSED;
        }
        for (stripe = 0; stripe < DVM_CELL_STRIPES; stripe++)
        {
            if (selected[stripe/8] & (1U << (stripe%8)))
            {
                keys[key_count++] = stripe;
            }
        }
    }
    original = access->count;
    for (item = 0; item < key_count; item++)
    {
        DVM_ObjectGuard *guard = &access->domain->stripes[keys[item]];
        if (selected_before(access, original, guard))
        {
            continue;
        }
        if (access->count == DVM_RANGE_GUARDS)
        {
            undo_additions(access, original);
            return DVM_CELL_REFUSED;
        }
        access->guards[access->count++] = guard;
    }
    for (item = original; item < access->count; item++)
    {
        DVM_ObjectResult result = dvm_object_guard_try_enter(access->guards[item]);
        if (result != DVM_OBJECT_ACQUIRED)
        {
            undo_additions(access, original);
            return result == DVM_OBJECT_BUSY ? DVM_CELL_BUSY : DVM_CELL_REFUSED;
        }
        access->held++;
    }
    for (item = original; item < access->count; item++)
    {
        DVM_ObjectGuard *guard = access->guards[item];
        unsigned int position = item;
        while (position && access->guards[position-1] > guard)
        {
            access->guards[position] = access->guards[position-1];
            position--;
        }
        access->guards[position] = guard;
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


/* Preserve historical two-span admission/empty/rollback behavior. source and
 * destination remain caller-owned; the ignored second span grants no access. */
DVM_CellResult dvm_range_access_begin(DVM_CellDomain *domain,
    const void *source, size_t source_size, void *destination, size_t destination_size,
    DVM_RangeAccess *access)
{
    return dvm_range_access_three_begin(domain, source, source_size, NULL, 0,
        destination, destination_size, access);
}
