/* Keep native arithmetic outside shared-memory guard intervals. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "real_arithmetic_value.h"
#include <stdlib.h>


/* Borrow owner-local saved pair/result only. Native FP environment and faults
 * are unchanged; caller holds no shared guards during this function. */
void dvm_runtime_real_arithmetic_value(int opcode, const void *pair, void *result)
{
    REAL left;
    REAL right;
    REAL value;
    memmove(&left, pair, sizeof(left));
    if (opcode != INEGF)
    {
        memmove(&right, (const unsigned char *)pair+sizeof(left), sizeof(right));
    }
    switch (opcode)
    {
    case IADDF:
        value = left + right;
        break;
    case ISUBF:
        value = left - right;
        break;
    case IMULF:
        value = left * right;
        break;
    case IDIVF:
        value = left / right;
        break;
    case INEGF:
        value = -left;
        break;
    default:
        abort();
    }
    memmove(result, &value, sizeof(value));
}


/* Publish saved result bytes into the caller-guarded output; no FP operation. */
void dvm_runtime_real_arithmetic_store(void *output, const void *result)
{
    memmove(output, result, sizeof(REAL));
}
