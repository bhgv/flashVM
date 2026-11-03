/* Allocation-free ordered stripe exclusion for possibly unaligned pointer cells. */
#include "disvm/cell_access.h"
#include "cell_access_private.h"
#include "disvm/refcount.h"
#include <stdint.h>
#include <stdlib.h>

#define STRIPES DVM_CELL_STRIPES /* Private shared domain geometry. */


/* Retire initialized private prefix after unpublished initialization failure. */
static void finalize(DVM_CellDomain *domain, unsigned int count)
{
    while (count)
    {
        count--;
        if (!dvm_object_guard_fini(&domain->stripes[count]))
        {
            abort();
        }
    }
    if (!dvm_object_domain_destroy(domain->parking))
    {
        abort();
    }
    free(domain);
}


/* Publish only after every inline guard is initialized. Caller owns result. */
DVM_CellDomain *dvm_cell_domain_create(void)
{
    DVM_CellDomain *domain = calloc(1, sizeof(*domain));
    unsigned int index;
    if (!domain)
    {
        return NULL;
    }
    domain->parking = dvm_object_domain_create();
    if (!domain->parking)
    {
        free(domain);
        return NULL;
    }
    for (index = 0; index < STRIPES; index++)
    {
        if (!dvm_object_guard_init(&domain->stripes[index], domain->parking))
        {
            finalize(domain, index);
            return NULL;
        }
    }
    domain->references = 1;
    return domain;
}


/* External quiescence excludes races with first lookup/last access return. */
int dvm_cell_domain_destroy(DVM_CellDomain **address)
{
    if (!address || !*address ||
        dvm_native_context_object_count(dvm_native_context_current()) ||
        dvm_refcount_load_u(&(*address)->references) != 1)
    {
        return 0;
    }
    finalize(*address, STRIPES);
    *address = NULL;
    return 1;
}


/* Roll back partial/complete access only on exact current owner. No callbacks.
 * Creator still owns domain, so dropping the access borrow must be nonfinal. */
static void unwind(DVM_CellAccess *access)
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


/* Verify private zeroed access before acquiring any native resource. */
static int empty(DVM_CellAccess *access)
{
    unsigned int index;
    if (!access || access->domain || access->context || access->count || access->held)
    {
        return 0;
    }
    for (index = 0; index < 4; index++)
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


/* Claim unique stripes in ascending index order. A failed claim drops all held
 * guards before returning; callers may retry only before ownership effects. */
DVM_CellResult dvm_cell_access_spans_begin(DVM_CellDomain *domain,
    const void *source, size_t source_size, void *destination, size_t destination_size,
    DVM_CellAccess *access)
{
    DVM_NativeContext *context = dvm_native_context_current();
    unsigned int keys[4];
    unsigned int index;
    unsigned int other;
    if (!domain || !context || dvm_native_context_object_count(context) ||
        !empty(access) || !range(source, source_size, keys) ||
        !range(destination, destination_size, keys + 2))
    {
        return DVM_CELL_REFUSED;
    }
    for (index = 0; index < 4; index++)
    {
        for (other = index + 1; other < 4; other++)
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
    for (index = 0; index < 4; index++)
    {
        if (index && keys[index] == keys[index - 1])
        {
            continue;
        }
        access->guards[access->count++] = &domain->stripes[keys[index]];
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


/* Foreign/depth refusal precedes mutation; owner may retry after extra unlock. */
int dvm_cell_access_end(DVM_CellAccess *access)
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


/* Pointer-cell compatibility entry; exact historical width/geometry semantics. */
DVM_CellResult dvm_cell_access_begin(DVM_CellDomain *domain,
    const void *source, void *destination, DVM_CellAccess *access)
{
    return dvm_cell_access_spans_begin(domain, source, sizeof(void *),
        destination, sizeof(void *), access);
}
