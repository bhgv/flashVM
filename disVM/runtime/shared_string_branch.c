/* Keep both owner slots, headers and common prefixes excluded until PC publication. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "raise.h"
#include "shared_string_branch.h"
#include "string_branch_value.h"
#include "branch_value.h"
#include "shared_access.h"
#include "managed_owner.h"
#include "runtime_gc.h"
#include "disvm/native_process.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


int dvm_runtime_parallel_string; /* Startup-only bounded string comparison experiment. */
enum { DVM_STRING_LEASE_BYTES = 128 }; /* Maximum copied prefix bytes per input. */


/* A local borrowed view; source guards retain its existing reachable owner. */
typedef struct StringView
{
    void *object;            /* Borrowed live String or H. */
    const void *data;        /* Borrowed common prefix after extension. */
    int encoded;            /* Original signed ASCII/Rune count. */
    int count;              /* Nonnegative validated character count. */
} StringView;


/* [GC:] view borrows an independently live owner, access holds its source cell.
 * Extend without releasing claims, then snapshot/validate len/max. Return static
 * error or NULL; access remains live on every return. No allocation/callback,
 * retain, root creation or payload read; caller always ends the access. */
static const char *header(StringView *view, DVM_RangeAccess *access, DVM_CellResult *claim)
{
    const void *field;
    size_t extent = offsetof(String, max)+sizeof(int);
    int capacity;
    DVM_CellResult result;
    if (!view->object)
    {
        return exNilref;
    }
    if (view->object == H)
    {
        view->encoded = 0;
        view->count = 0;
        return NULL;
    }
    if ((uintptr_t)view->object > (uintptr_t)-1-extent)
    {
        return exHeap;
    }
    field = (const unsigned char *)view->object+offsetof(String, len);
    result = dvm_range_access_extend(access, field, extent - offsetof(String, len));
    if (claim)
    {
        *claim = result;
    }
    if (result != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    dvm_runtime_string_branch_header(view->object, &view->encoded, &capacity);
    if (view->encoded == (-0x7fffffff-1))
    {
        return exHeap;
    }
    view->count = view->encoded < 0 ? -view->encoded : view->encoded;
    return capacity < view->count ? exHeap : NULL;
}


/* view has a guarded validated header; count is the shared prefix length.
 * Add that complete payload before any key read. Return error/NULL with access
 * still live. No allocation, owner change, callback or payload read. */
static const char *prefix(StringView *view, size_t count, DVM_RangeAccess *access)
{
    uintptr_t address;
    size_t unit = view->encoded < 0 ? sizeof(Rune) : 1;
    if (!count)
    {
        return NULL;
    }
    address = (uintptr_t)view->object;
    if (address > (uintptr_t)-1-offsetof(String, data) || count > (size_t)-1/unit)
    {
        return exHeap;
    }
    address += offsetof(String, data);
    if (count*unit > (uintptr_t)-1-address)
    {
        return exHeap;
    }
    view->data = (const void *)address;
    return dvm_range_access_extend(access, view->data, count*unit) == DVM_CELL_ACQUIRED ? NULL : exHeap;
}


/* Compare guarded or caller-private views over the common prefix. */
static int compare_prefix(const StringView *left, const StringView *right, size_t count)
{
    size_t index;

    for (index = 0; index < count; index++)
    {
        uint32_t a = dvm_runtime_string_branch_key(left->data, index, left->encoded < 0);
        uint32_t b = dvm_runtime_string_branch_key(right->data, index, right->encoded < 0);
        if (a != b)
        {
            return (a ^ UINT32_C(0x80000000)) < (b ^ UINT32_C(0x80000000)) ? -1 : 1;
        }
    }
    return (left->count > right->count) - (left->count < right->count);
}


/* Interpret the retained six comparison conditions. */
static int branch_taken(int condition, int order)
{
    return condition == 0 ? order == 0 : condition == 1 ? order != 0 :
        condition == 2 ? order < 0 : condition == 3 ? order <= 0 :
        condition == 4 ? order > 0 : order >= 0;
}


/* Consume the acquired source union and compare bounded private prefixes.
 * The active Prog retains the immediate target cell throughout the lease.
 * Only a taken branch reacquires and reads that cell. */
static const char *leased_string_branch(int opcode, int condition, StringView left, StringView right,
    size_t count, size_t left_bytes, size_t right_bytes, const void *target, void *pc,
    DVM_RangeAccess *access)
{
    unsigned char left_copy[DVM_STRING_LEASE_BYTES], right_copy[DVM_STRING_LEASE_BYTES];
    int released, order;
    DVM_CellResult claim;
    const char *problem;

    memcpy(left_copy, left.data, left_bytes);
    memcpy(right_copy, right.data, right_bytes);
    left.data = left_copy;
    right.data = right_copy;
    left.object = NULL;
    right.object = NULL;
    if (!dvm_range_access_end(access))
    {
        abort();
    }
    released = dvm_runtime_gc_native_try_leave();
    order = compare_prefix(&left, &right, count);
    if (released > 0)
    {
        dvm_runtime_gc_native_enter(released);
    }
    if (!branch_taken(condition, order))
    {
        return NULL;
    }
    claim = dvm_managed_ranges_begin(target, sizeof(Inst *), NULL, 0, access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : exHeap;
    }
    problem = dvm_runtime_branch_target(target, pc) ? NULL : exHeap;
    if (!dvm_range_access_end(access))
    {
        abort();
    }
    return problem;
}


/* [PERF STRINGCMP-105] Whole-profile timing and candidates are retained in
 * PERFORMANCE-COSTS.md; no isolated component attribution.
 * [GC:] condition/slots/pc follow shared_string_branch.h. Keep source owners and
 * every discovered span excluded until the conditional target snapshot/store.
 * Error returns only after complete end. Long prefixes may occupy all stripes;
 * no collector admission, provenance, root or code lifetime is inferred. */
const char *dvm_runtime_string_branch(int condition, const void *source,
    const void *other, const void *target, void *pc)
{
    static const int opcodes[] = {IBEQC, IBNEC, IBLTC, IBLEC, IBGTC, IBGEC}; /* Condition to Dis opcode. */
    DVM_RangeAccess access = {0};
    DVM_CellResult claim;
    StringView left = {0};
    StringView right = {0};
    const char *problem;
    int invalid_target;
    int order = 0;
    int take;
    size_t count;
    if (dvm_runtime_shared_mode() != 2 || pc != &DVM_R.PC || condition < 0 || condition > 5)
    {
        return exHeap;
    }
    if (!source || source == H || (uintptr_t)source > (uintptr_t)-1-sizeof(void *) ||
        !other || other == H || (uintptr_t)other > (uintptr_t)-1-sizeof(void *))
    {
        return exHeap;
    }
    invalid_target = !target || target == H || (uintptr_t)target > (uintptr_t)-1-sizeof(Inst *);
    claim = dvm_managed_three_begin(source, sizeof(void *), other, sizeof(void *),
        invalid_target ? (void *)source : (void *)target, sizeof(void *), &access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcodes[condition]) ? NULL : exHeap;
    }
    left.object = dvm_runtime_string_branch_source(source);
    right.object = dvm_runtime_string_branch_source(other);
    /* NULL on either side precedes header access, matching retained stringcmp. */
    problem = (!left.object || !right.object) ? exNilref : NULL;
    if (!problem)
    {
        problem = header(&left, &access, NULL);
    }
    if (!problem)
    {
        problem = header(&right, &access, NULL);
    }
    if (!problem && left.object != right.object)
    {
        count = left.count < right.count ? left.count : right.count;
        problem = prefix(&left, count, &access);
        if (!problem)
        {
            problem = prefix(&right, count, &access);
        }
        if (!problem)
        {
            size_t left_bytes = count * (left.encoded < 0 ? sizeof(Rune) : 1);
            size_t right_bytes = count * (right.encoded < 0 ? sizeof(Rune) : 1);
            if (dvm_runtime_parallel_string && count &&
                left_bytes <= DVM_STRING_LEASE_BYTES && right_bytes <= DVM_STRING_LEASE_BYTES &&
                dvm_native_execution()->arithmetic_retry_opcode == opcodes[condition] &&
                dvm_runtime_instruction_target(target))
            {
                return leased_string_branch(opcodes[condition], condition, left, right, count,
                    left_bytes, right_bytes, target, pc, &access);
            }
            order = compare_prefix(&left, &right, count);
        }
    }
    if (!problem)
    {
        take = branch_taken(condition, order);
        if (take && (invalid_target || !dvm_runtime_branch_target(target, pc)))
        {
            problem = exHeap;
        }
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return problem;
}



/* Guard the reference, index and result together, then extend the same
 * union over the borrowed header and selected character. No lease escapes. */
const char *dvm_runtime_string_index(const void *source, const void *index_cell, void *destination)
{
    DVM_RangeAccess access = {0};
    DVM_CellResult claim;
    StringView view = {0};
    const char *problem = NULL;
    WORD index, result;
    size_t unit;
    uintptr_t address;
    uint32_t rune;

    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    claim = dvm_managed_three_begin(source, sizeof(void *), index_cell, sizeof(index),
        destination, sizeof(result), &access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(IINDC) ? NULL : exHeap;
    }
    memmove(&index, index_cell, sizeof(index));
    view.object = dvm_runtime_string_branch_source(source);
    if (!view.object || view.object == H)
    {
        problem = exNilref;
    }
    else if ((uintptr_t)view.object > UINTPTR_MAX - offsetof(String, data))
    {
        problem = exHeap;
    }
    else
    {
        problem = header(&view, &access, &claim);
    }
    if (!problem)
    {
        if (index < 0)
        {
            index += view.count;
        }
        if (index < 0 || index >= view.count)
        {
            problem = exBounds;
        }
    }
    if (!problem)
    {
        unit = view.encoded < 0 ? sizeof(Rune) : 1;
        address = (uintptr_t)view.object + offsetof(String, data);
        if ((size_t)index >= (UINTPTR_MAX - address) / unit)
        {
            problem = exHeap;
        }
        else
        {
            const void *character = (const void *)(address + (size_t)index * unit);
            claim = dvm_range_access_extend(&access, character, unit);
            if (claim != DVM_CELL_ACQUIRED)
            {
                problem = exHeap;
            }
            else
            {
                rune = dvm_runtime_string_branch_key(character, 0, view.encoded < 0);
                result = rune & UINT32_C(0x80000000) ?
                    -(WORD)(~rune & UINT32_C(0x7fffffff)) - 1 : (WORD)rune;
                memmove(destination, &result, sizeof(result));
            }
        }
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(IINDC) ? NULL : problem;
}
