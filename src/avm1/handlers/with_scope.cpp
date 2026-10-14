/* FlashVM: value-based lexical With scopes shared by interpreted and generated bodies. */
#include "support.hpp"
#include "with_scope_constants.h"
/* [GC:] Build a managed scope node without exposing bytecode or an operand stack. */
extern "C" FVM_Result fvm_exec_with_scope(FVM_Context *c, FVM_Object *parent,
                                          const FVM_Value *input, FVM_Object **out)
{
    if (!c || !out || (parent && parent->owner != c) || fvm_validate_value(c, input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_WITH_SCOPE_ROOT_COUNT);
        roots.values[FVM_WITH_SCOPE_VALUE_ROOT] = *input;
        if (parent)
        {
            roots.values[FVM_WITH_SCOPE_PARENT_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_WITH_SCOPE_PARENT_ROOT].object = parent;
        }
        if (input->type == FVM_VALUE_UNDEFINED || input->type == FVM_VALUE_NULL)
        {
            *out = parent;
            return FVM_OK;
        }
        if (input->type != FVM_VALUE_OBJECT)
        {
            FVM_Value &wrapped = roots.values[FVM_WITH_SCOPE_VALUE_ROOT];
            FVM_Result status = fvm_exec_to_object(c, &wrapped, &wrapped);
            if (status != FVM_OK)
                return status;
            if (wrapped.type != FVM_VALUE_OBJECT)
            {
                /* An unavailable loaded class still executes With on a fresh bare object. */
                FVM_Object *bare = 0;
                status = fvm_object_create(c, &bare);
                if (status != FVM_OK)
                    return status;
                wrapped.type = FVM_VALUE_OBJECT;
                wrapped.object = bare;
            }
        }
        FVM_Object *node = 0;
        FVM_Result status = fvm_object_create(c, &node);
        if (status != FVM_OK)
            return status;
        node->resource_kind = FVM_RESOURCE_WITH_SCOPE;
        node->resource_result_valid = true;
        /* [GC:] The managed state and prototype retain the object and enclosing scope. */
        node->resource_result = roots.values[FVM_WITH_SCOPE_VALUE_ROOT];
        node->prototype = parent;
        *out = node;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
