/* FlashVM: CharToAscii/MBCharToAscii over explicit rooted values. */
#include "support.hpp"
#include "string_operation_constants.h"
#include "../../core/unicode_constants.h"
/* Return the first UTF-16 unit, replacing surrogate units and retaining legacy bounds. */
extern "C" FVM_Result fvm_exec_char_code(FVM_Context *c, const FVM_Value *input, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Retain the receiver through user-controlled string coercion. */
        FVM_ExecutionRoots roots(c, FVM_STRING_UNARY_ROOT_COUNT);
        roots.values[FVM_STRING_UNARY_INPUT_ROOT] = *input;
        FVM_Value text;
        FVM_Result status = fvm_avm1_string(c, roots.values[FVM_STRING_UNARY_INPUT_ROOT], &text);
        if (status != FVM_OK)
            return status;
        fvm_detail::vector<unsigned int> units;
        status = fvm_string_units(text, units);
        if (status != FVM_OK)
            return status;
        unsigned int code = units.empty() ? 0 : units[0];
        if (c->swf_loaded && c->swf_info.version < FVM_SWF_VERSION_6 &&
            code >= FVM_UTF8_ASCII_LIMIT)
            return FVM_UNSUPPORTED;
        if (code >= FVM_UNICODE_SURROGATE_MIN && code <= FVM_UNICODE_SURROGATE_MAX)
            code = FVM_UNICODE_REPLACEMENT_CHARACTER;
        fvm_value_number(out, code);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
