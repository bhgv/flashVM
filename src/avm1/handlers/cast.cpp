/* FlashVM: value-based CastOp preserving primitive boxing side effects. */
#include "support.hpp"
#include "cast_constants.h"
#include "flashvm/standard.h"
/* Return the original matching value or null; loaded primitive boxing runs even on mismatch. */
extern "C" FVM_Result fvm_exec_cast(FVM_Context *c, const FVM_Value *value,
                                    const FVM_Value *constructor, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, value) != FVM_OK ||
        fvm_validate_value(c, constructor) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_CAST_ROOT_COUNT);
        roots.values[FVM_CAST_VALUE_ROOT] = *value;
        roots.values[FVM_CAST_CLASS_ROOT] = *constructor;
        int match = 0;
        FVM_Result status;
        if (c->swf_loaded && value->type != FVM_VALUE_OBJECT)
        {
            /* The wrapper is deliberately discarded: the original primitive never matches. */
            status = fvm_exec_to_object(c, &roots.values[FVM_CAST_VALUE_ROOT],
                                        &roots.values[FVM_CAST_BOX_ROOT]);
        }
        else
            status = fvm_value_instance_of(c, &roots.values[FVM_CAST_VALUE_ROOT],
                                           &roots.values[FVM_CAST_CLASS_ROOT], &match);
        if (status != FVM_OK)
            return status;
        FVM_Value result;
        if (match)
            result = roots.values[FVM_CAST_VALUE_ROOT];
        else
        {
            fvm_value_undefined(&result);
            result.type = FVM_VALUE_NULL;
        }
        *out = result;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
