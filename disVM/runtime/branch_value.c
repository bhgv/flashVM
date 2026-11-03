/* Integer comparison snapshots; no FP state, reference or allocation effects. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "branch_value.h"
#include <stdlib.h>


/* Borrow jointly guarded source/other, decode opcode width and compare complete
 * snapshots. Return boolean; no callback, allocation or externally visible store.
 * Invalid internal opcode is a fatal programming error, not a guest fallback. */
int dvm_runtime_branch_predicate(int opcode, const void *source, const void *other)
{
    switch (opcode)
    {
    case IBEQB:
    {
        BYTE left;
        BYTE right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left == right;
    }
    case IBNEB:
    {
        BYTE left;
        BYTE right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left != right;
    }
    case IBLTB:
    {
        BYTE left;
        BYTE right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left < right;
    }
    case IBLEB:
    {
        BYTE left;
        BYTE right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left <= right;
    }
    case IBGTB:
    {
        BYTE left;
        BYTE right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left > right;
    }
    case IBGEB:
    {
        BYTE left;
        BYTE right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left >= right;
    }
    case IBEQW:
    {
        WORD left;
        WORD right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left == right;
    }
    case IBNEW:
    {
        WORD left;
        WORD right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left != right;
    }
    case IBLTW:
    {
        WORD left;
        WORD right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left < right;
    }
    case IBLEW:
    {
        WORD left;
        WORD right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left <= right;
    }
    case IBGTW:
    {
        WORD left;
        WORD right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left > right;
    }
    case IBGEW:
    {
        WORD left;
        WORD right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left >= right;
    }
    case IBEQL:
    {
        LONG left;
        LONG right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left == right;
    }
    case IBNEL:
    {
        LONG left;
        LONG right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left != right;
    }
    case IBLTL:
    {
        LONG left;
        LONG right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left < right;
    }
    case IBLEL:
    {
        LONG left;
        LONG right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left <= right;
    }
    case IBGTL:
    {
        LONG left;
        LONG right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left > right;
    }
    case IBGEL:
    {
        LONG left;
        LONG right;
        memmove(&left, source, sizeof(left));
        memmove(&right, other, sizeof(right));
        return left >= right;
    }
    default:
        abort();
    }
}


/* Borrow target and owner-local pc; capture the whole pointer before publishing.
 * Return0 on nil/H without effects,1 after success. No code/heap dereference. */
int dvm_runtime_branch_target(const void *target, void *pc)
{
    Inst *value;
    memmove(&value, target, sizeof(value));
    if (!value || value == H)
    {
        return 0;
    }
    memmove(pc, &value, sizeof(value));
    return 1;
}
