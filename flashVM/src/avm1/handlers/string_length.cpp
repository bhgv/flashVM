/* FlashVM: StringLength/MBStringLength over explicit rooted values. */
#include "support.hpp"
#include "string_operation_constants.h"
#include "../../core/unicode_constants.h"


/* Count UTF-16 units; retain the supported legacy ASCII-only boundary. */
extern "C" FVM_Result fvm_exec_string_length(FVM_Context *c, const FVM_Value *input, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, input) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Coercion callbacks may detach or collect their input object. */
        FVM_ExecutionRoots roots(c, FVM_STRING_UNARY_ROOT_COUNT);
        roots.values[FVM_STRING_UNARY_INPUT_ROOT] = *input;
        FVM_Value text;
        FVM_Result status = fvm_avm1_string(c, roots.values[FVM_STRING_UNARY_INPUT_ROOT], &text);
        if (status != FVM_OK)
        {
            return status;
        }
        fvm_detail::vector<unsigned int> units;
        status = fvm_string_units(text, units);
        if (status != FVM_OK)
        {
            return status;
        }
        if (c->swf_loaded && c->swf_info.version < FVM_SWF_VERSION_6)
        {
            for (size_t i = 0; i < units.size(); ++i)
            {
                if (units[i] >= FVM_UTF8_ASCII_LIMIT)
                {
                    return FVM_UNSUPPORTED;
                }
            }
        }
        fvm_value_number(out, (double)units.size());
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
