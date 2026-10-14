/* FlashVM: primitive boxing through explicit values for interpreter and generated code. */
#include "support.hpp"
#include "to_object_constants.h"
#include "flashvm/standard.h"
/* Return objects unchanged, undefined for nullish/unavailable loaded constructors, or a wrapper. */
extern "C" FVM_Result fvm_exec_to_object(FVM_Context *c, const FVM_Value *input, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    if (input->type == FVM_VALUE_OBJECT)
    {
        *out = *input;
        return FVM_OK;
    }
    if (input->type == FVM_VALUE_UNDEFINED || input->type == FVM_VALUE_NULL)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    try
    {
        FVM_ExecutionRoots roots(c, FVM_TO_OBJECT_ROOT_COUNT);
        roots.values[FVM_TO_OBJECT_INPUT_ROOT] = *input;
        const char *name = input->type == FVM_VALUE_STRING   ? "String"
                           : input->type == FVM_VALUE_NUMBER ? "Number"
                                                             : "Boolean";
        FVM_Value &result = roots.values[FVM_TO_OBJECT_RESULT_ROOT];
        FVM_Result status;
        if (c->swf_loaded)
        {
            FVM_Value &key = roots.values[FVM_TO_OBJECT_NAME_ROOT];
            status = fvm_value_string(&key, name);
            if (status == FVM_OK)
                status =
                    fvm_exec_new_object(c, 0, c->global, 0, 0, &key, FVM_TO_OBJECT_ARGUMENT_COUNT,
                                        &roots.values[FVM_TO_OBJECT_INPUT_ROOT], &result);
        }
        else
        {
            FVM_Object *boxed = 0;
            status = fvm_construct(c, name, FVM_TO_OBJECT_ARGUMENT_COUNT,
                                   &roots.values[FVM_TO_OBJECT_INPUT_ROOT], &boxed);
            if (status == FVM_OK)
            {
                result.type = FVM_VALUE_OBJECT;
                result.object = boxed;
            }
        }
        if (status == FVM_OK)
            *out = result;
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
