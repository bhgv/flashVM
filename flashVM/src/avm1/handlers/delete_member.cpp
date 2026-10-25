/* FlashVM: value-based property deletion shared by interpreter and generated code. */
#include "support.hpp"
#include "value_operation_constants.h"


/* Delete an own member after version-aware name conversion; absence reports false. */
extern "C" FVM_Result
fvm_exec_delete_member(FVM_Context *c, const FVM_Value *owner_value, const FVM_Value *name_value, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, owner_value) != FVM_OK || fvm_validate_value(c, name_value) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        FVM_ExecutionRoots roots(c, FVM_EXEC_VALUE_DELETE_ROOT_COUNT);
        roots.values[FVM_EXEC_VALUE_LEFT_ROOT] = *owner_value;
        roots.values[FVM_EXEC_VALUE_RIGHT_ROOT] = *name_value;
        FVM_Value &key = roots.values[FVM_EXEC_VALUE_TEMP_ROOT];
        FVM_Result status = fvm_avm1_string(c, roots.values[FVM_EXEC_VALUE_RIGHT_ROOT], &key);
        if (status != FVM_OK)
        {
            return status;
        }
        const FVM_Value &owner = roots.values[FVM_EXEC_VALUE_LEFT_ROOT];
        int own = 0, deleted = 0;
        if (owner.type == FVM_VALUE_OBJECT && owner.object->resource_kind != FVM_RESOURCE_SUPER)
        {
            status = fvm_object_has_own_bytes(c, owner.object, key.string, fvm_string_size(key), &own);
            if (status == FVM_OK && own)
            {
                status = fvm_object_delete_bytes(c, owner.object, key.string, fvm_string_size(key), &deleted);
            }
            if (status != FVM_OK)
            {
                return status;
            }
        }
        fvm_value_boolean(out, deleted);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
