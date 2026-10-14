/* FlashVM: value-based lexical With scopes shared by interpreted and generated bodies. */
#include "support.hpp"
#include "flashvm/standard.h"
#include "../../core/property_key.hpp"
/* [GC:] Build a managed scope node without exposing bytecode or an operand stack. */
extern "C" FVM_Result fvm_exec_with_scope(FVM_Context *c, FVM_Object *parent,
                                          const FVM_Value *input, FVM_Object **out)
{
    if (!c || !out || (parent && parent->owner != c) || fvm_validate_value(c, input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, 2);
        roots.values[0] = *input;
        if (parent)
        {
            roots.values[1].type = FVM_VALUE_OBJECT;
            roots.values[1].object = parent;
        }
        if (input->type == FVM_VALUE_UNDEFINED || input->type == FVM_VALUE_NULL)
        {
            *out = parent;
            return FVM_OK;
        }
        if (input->type != FVM_VALUE_OBJECT)
        {
            const char *type = input->type == FVM_VALUE_STRING   ? "String"
                               : input->type == FVM_VALUE_NUMBER ? "Number"
                                                                 : "Boolean";
            FVM_Object *boxed = 0;
            FVM_Result status = fvm_construct(c, type, 1, &roots.values[0], &boxed);
            if (status != FVM_OK)
                return status;
            roots.values[0].type = FVM_VALUE_OBJECT;
            roots.values[0].object = boxed;
        }
        FVM_Object *node = 0;
        FVM_Result status = fvm_object_create(c, &node);
        if (status != FVM_OK)
            return status;
        node->resource_kind = FVM_RESOURCE_WITH_SCOPE;
        node->resource_result_valid = true;
        /* [GC:] The managed state and prototype retain the object and enclosing scope. */
        node->resource_result = roots.values[0];
        node->prototype = parent;
        *out = node;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
