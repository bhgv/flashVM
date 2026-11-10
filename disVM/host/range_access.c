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
        DVM_ObjectResult result = domain->blocking ? dvm_object_guard_enter(access->guards[index]) :
            dvm_object_guard_try_enter(access->guards[index]);
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
    DVM_CellResult result = begin_selected(domain, bits, access);
    if (result == DVM_CELL_ACQUIRED && (dvm_cell_write_reserved(domain, destination, destination_size) ||
        dvm_cell_read_reserved(domain, source, source_size) || dvm_cell_read_reserved(domain, second, second_size)))
    {
        unwind(access);
        return DVM_CELL_BUSY;
    }
    return result;
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
    DVM_CellResult result = dvm_range_plan_begin(domain, &plan, access);
    if (result == DVM_CELL_ACQUIRED &&
        __atomic_load_n(&domain->reservation_active, __ATOMIC_ACQUIRE) == 2)
    {
        for (size_t index = 0; index < count; index++)
        {
            if (dvm_cell_read_reserved(domain, cells[index], sizeof(void *)))
            {
                unwind(access);
                return DVM_CELL_BUSY;
            }
        }
    }
    return result;
}


/* EN: Register the current observer in domain for the lifetime of watch.
 * RU: Зарегистрировать текущего наблюдателя в domain на время жизни watch. */
int dvm_range_watch_begin(DVM_CellDomain *domain, DVM_RangeWatch *watch)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_NativeContext *expected = NULL;
    if (!domain || !context || !watch || watch->domain ||
        dvm_refcount_retain_u(&domain->references) != DVM_REFCOUNT_LIVE)
    {
        return 0;
    }
    if (!__atomic_compare_exchange_n(&domain->watch_owner, &expected, context, 0,
            __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    {
        if (dvm_refcount_release_u(&domain->references) != DVM_REFCOUNT_LIVE)
        {
            abort();
        }
        return 0;
    }
    *watch = (DVM_RangeWatch){.domain = domain, .context = context};
    return 1;
}


/* EN: Check that watch and held access belong to this context and domain.
 * RU: Проверить принадлежность watch и захваченного access контексту и домену. */
static int watch_access_matches(const DVM_RangeWatch *watch, const DVM_RangeAccess *access)
{
    return watch && access && watch->domain && access->domain == watch->domain &&
        access->context == watch->context && watch->context == dvm_native_context_current() &&
        access->held == access->count;
}


/* EN: Add held access stripes to watch, refusing any changed observation.
 * RU: Добавить захваченные полосы access в watch, отклоняя изменившиеся версии. */
int dvm_range_watch_sample(DVM_RangeWatch *watch, const DVM_RangeAccess *access)
{
    if (!watch_access_matches(watch, access))
    {
        return 0;
    }
    for (unsigned int index = 0; index < access->held; index++)
    {
        size_t key = access->guards[index] - watch->domain->stripes;
        unsigned char bit = 1U << (key % 8);
        unsigned long revision = watch->domain->revision[key];
        if (revision == ULONG_MAX ||
            ((watch->sampled.bits[key / 8] & bit) && watch->revision[key] != revision))
        {
            return 0;
        }
        watch->sampled.bits[key / 8] |= bit;
        watch->revision[key] = revision;
    }
    return 1;
}


/* EN: Recheck all held access stripes against watch without reading fields.
 * RU: Сверить захваченные полосы access с watch без чтения самих полей. */
int dvm_range_watch_unchanged(const DVM_RangeWatch *watch, const DVM_RangeAccess *access)
{
    if (!watch_access_matches(watch, access))
    {
        return 0;
    }
    for (unsigned int index = 0; index < access->held; index++)
    {
        size_t key = access->guards[index] - watch->domain->stripes;
        if (!(watch->sampled.bits[key / 8] & (1U << (key % 8))) ||
            watch->domain->revision[key] != watch->revision[key])
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Withdraw watch from its borrowed domain on the original context.
 * RU: Снять наблюдение watch с домена в исходном контексте. */
int dvm_range_watch_end(DVM_RangeWatch *watch)
{
    if (!watch || !watch->domain || !watch->context ||
        watch->context != dvm_native_context_current())
    {
        return 0;
    }
    DVM_CellDomain *domain = watch->domain;
    __atomic_store_n(&domain->watch_owner, NULL, __ATOMIC_RELEASE);
    if (dvm_refcount_release_u(&domain->references) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
    watch->domain = NULL;
    watch->context = NULL;
    return 1;
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
    if (dvm_cell_read_reserved(access->domain, span, size))
    {
        undo_additions(access, original);
        return DVM_CELL_BUSY;
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
