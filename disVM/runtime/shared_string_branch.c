/* Keep both owner slots, headers and common prefixes excluded until PC publication. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "raise.h"
#include "shared_string_branch.h"
#include "string_branch_value.h"
#include "branch_value.h"
#include "shared_access.h"
#include "managed_owner.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


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
static const char *header(StringView *view, DVM_RangeAccess *access)
{
    const void *field;
    size_t extent = offsetof(String, max)+sizeof(int);
    int capacity;
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
    if (dvm_range_access_extend(access, field, extent-offsetof(String, len)) != DVM_CELL_ACQUIRED)
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


/* [PERF STRINGCMP-105] Whole-profile timing and candidates are retained in
 * PERFORMANCE-COSTS.md; no isolated component attribution.
 * [GC:] condition/slots/pc follow shared_string_branch.h. Keep source owners and
 * every discovered span excluded until the conditional target snapshot/store.
 * Error returns only after complete end. Long prefixes may occupy all stripes;
 * no collector admission, provenance, root or code lifetime is inferred. */
const char *dvm_runtime_string_branch(int condition, const void *source,
    const void *other, const void *target, void *pc)
{
    DVM_RangeAccess access = {0};
    StringView left = {0};
    StringView right = {0};
    const char *problem;
    int invalid_target;
    int order = 0;
    int take;
    size_t count;
    size_t index;
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
    if (dvm_managed_three_begin(source, sizeof(void *), other, sizeof(void *),
        invalid_target ? (void *)source : (void *)target, sizeof(void *), &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    left.object = dvm_runtime_string_branch_source(source);
    right.object = dvm_runtime_string_branch_source(other);
    /* NULL on either side precedes header access, matching retained stringcmp. */
    problem = (!left.object || !right.object) ? exNilref : NULL;
    if (!problem)
    {
        problem = header(&left, &access);
    }
    if (!problem)
    {
        problem = header(&right, &access);
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
            for (index = 0; index < count; index++)
            {
                uint32_t a = dvm_runtime_string_branch_key(left.data, index, left.encoded < 0);
                uint32_t b = dvm_runtime_string_branch_key(right.data, index, right.encoded < 0);
                if (a != b)
                {
                    order = (a^UINT32_C(0x80000000)) < (b^UINT32_C(0x80000000)) ? -1 : 1;
                    break;
                }
            }
            if (!order)
            {
                order = (left.count > right.count)-(left.count < right.count);
            }
        }
    }
    if (!problem)
    {
        take = condition == 0 ? order == 0 : condition == 1 ? order != 0 :
            condition == 2 ? order < 0 : condition == 3 ? order <= 0 :
            condition == 4 ? order > 0 : order >= 0;
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
