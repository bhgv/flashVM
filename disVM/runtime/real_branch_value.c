/* Never perform floating-point evaluation until shared input guards are gone. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "real_branch_value.h"
#include <stdlib.h>


/* Borrow jointly guarded source/other and owner-local two-REAL byte storage.
 * Snapshot both complete representations without FP operations, callbacks,
 * allocation or reference changes. Return void after the two copies. */
void dvm_runtime_real_branch_capture(const void *source, const void *other, void *pair)
{
    memmove(pair, source, sizeof(REAL));
    memmove((unsigned char *)pair+sizeof(REAL), other, sizeof(REAL));
}


/* Borrow owner-local pair; condition is validated0..5. Evaluate native REAL
 * comparison outside all guards, retaining current FP exception semantics.
 * Return boolean; no shared accesses/ownership changes. Unknown condition aborts. */
int dvm_runtime_real_branch_predicate(int condition, const void *pair)
{
    REAL left;
    REAL right;
    memmove(&left, pair, sizeof(left));
    memmove(&right, (const unsigned char *)pair+sizeof(left), sizeof(right));
    switch (condition)
    {
    case 0:
        return left == right;
    case 1:
        return left != right;
    case 2:
        return left < right;
    case 3:
        return left <= right;
    case 4:
        return left > right;
    case 5:
        return left >= right;
    default:
        abort();
    }
}
