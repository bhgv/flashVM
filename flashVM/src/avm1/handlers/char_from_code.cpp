/* FlashVM: AsciiToChar/MBAsciiToChar over explicit rooted values. */
#include "support.hpp"
#include "string_operation_constants.h"
#include "../../core/unicode_constants.h"


/* Wrap to Uint16; zero becomes empty and surrogate units become replacement characters. */
extern "C" FVM_Result fvm_exec_char_from_code(FVM_Context *c, const FVM_Value *input, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, input) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Keep the numeric-coercion receiver alive until the operation completes. */
        FVM_ExecutionRoots roots(c, FVM_STRING_UNARY_ROOT_COUNT);
        roots.values[FVM_STRING_UNARY_INPUT_ROOT] = *input;
        FVM_Value number;
        FVM_Result status = fvm_exec_to_number(c, 1, &roots.values[FVM_STRING_UNARY_INPUT_ROOT], &number);
        if (status != FVM_OK)
        {
            return status;
        }
        unsigned short code = (unsigned short)((unsigned long)(long)number.number & FVM_UTF16_UNIT_MASK);
        if (c->swf_loaded && c->swf_info.version < FVM_SWF_VERSION_6 && code >= FVM_UTF8_ASCII_LIMIT)
        {
            return FVM_UNSUPPORTED;
        }
        if (code >= FVM_UNICODE_SURROGATE_MIN && code <= FVM_UNICODE_SURROGATE_MAX)
        {
            code = FVM_UNICODE_REPLACEMENT_CHARACTER;
        }
        FVM_Value result;
        status = fvm_value_string_utf16(c, &code, code ? 1 : 0, &result);
        if (status == FVM_OK)
        {
            *out = result;
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
