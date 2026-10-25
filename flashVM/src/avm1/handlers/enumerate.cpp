/* FlashVM: value-based property-name snapshots shared by interpreter and generated bodies. */
#include "support.hpp"
#include "enumerate_constants.h"
#include "../../standard/base_state.hpp"


/* Snapshot enumerable object names; loaded primitives produce no names and are never boxed. */
extern "C" FVM_Result fvm_exec_enumerate(FVM_Context *context, const FVM_Value *input, FVM_Value *output)
{
    if (!context || context->destroying || context->gc_releasing || !output ||
        fvm_validate_value(context, input) != FVM_OK || (!context->swf_loaded && input->type != FVM_VALUE_OBJECT))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Retain the original value until the native key snapshot is completely built. */
        FVM_ExecutionRoots roots(context, FVM_EXEC_ENUMERATE_ROOT_COUNT);
        roots.values[FVM_EXEC_ENUMERATE_INPUT_ROOT] = *input;
        FVM_Object *keys = 0;
        FVM_Result status =
            input->type == FVM_VALUE_OBJECT
                ? fvm_object_enumerate(context, input->object, FVM_EXEC_ENUMERATE_INCLUDE_PROTOTYPES, &keys)
                : fvm_array_create_like(context, 0, 0, &keys, true);
        if (status != FVM_OK)
        {
            return status;
        }
        fvm_value_undefined(output);
        output->type = FVM_VALUE_OBJECT;
        output->object = keys;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
