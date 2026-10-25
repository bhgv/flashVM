/* FlashVM: version-aware truth conversion for C callers and generated branches. */
#include "support.hpp"


/* Truth conversion does not execute user callbacks; pre-SWF7 strings use numeric parsing. */
extern "C" FVM_Result fvm_exec_to_boolean(FVM_Context *c, const FVM_Value *value, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, value) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {
        bool converted = false;
        FVM_Result status = fvm_avm1_boolean(c, *value, &converted);
        if (status == FVM_OK)
        {
            fvm_value_boolean(out, converted);
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
