/* Checked width-specific tails for the existing x64 scalar constructors. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "scalar_cons.h"
#include "owner_fast.h"
#include "disvm/native_context.h"


/* [GC:] Reject foreign/inactive/guarded REG before reading decoded operands.
 * width/extent are process-live constants. Return0 before effects or completed1;
 * propagated VM error never becomes a decline/retry. No cache reads after call. */
static int invoke(REG *registers, size_t width, size_t extent)
{
    if (!dvm_runtime_owner_entry_ready(registers) ||
        dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    return dvm_runtime_scalar_cons(registers->s, registers->d, width, extent);
}


/* Byte register profile; preserve original padded byte-head allocation width. */
int dvm_runtime_scalar_1_fast(REG *registers)
{
    return invoke(registers, 1, IBY2WD);
}


/* Four-byte register profile; copy exact scalar bits without numeric conversion. */
int dvm_runtime_scalar_4_fast(REG *registers)
{
    return invoke(registers, 4, 4);
}


/* Eight-byte WORD/LONG/REAL profile; preserve exact bits including NaN payload. */
int dvm_runtime_scalar_8_fast(REG *registers)
{
    return invoke(registers, 8, 8);
}
