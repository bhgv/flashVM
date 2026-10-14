/* FlashVM: StringAdd over rooted values, with right-before-left string conversion. */
#include "support.hpp"
#include "string_operation_constants.h"
/* Concatenate exact counted bytes and publish a context-owned result after both conversions. */
extern "C" FVM_Result fvm_exec_string_add(FVM_Context *c, const FVM_Value *left,
                                          const FVM_Value *right, FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, left) != FVM_OK ||
        fvm_validate_value(c, right) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Original objects and newly allocated text must survive the other callback. */
        FVM_ExecutionRoots roots(c, FVM_STRING_BINARY_ROOT_COUNT);
        roots.values[FVM_STRING_BINARY_LEFT_ROOT] = *left;
        roots.values[FVM_STRING_BINARY_RIGHT_ROOT] = *right;
        FVM_Value &a = roots.values[FVM_STRING_BINARY_LEFT_TEXT_ROOT];
        FVM_Value &b = roots.values[FVM_STRING_BINARY_RIGHT_TEXT_ROOT];
        FVM_Result status = fvm_avm1_string(c, roots.values[FVM_STRING_BINARY_RIGHT_ROOT], &b);
        if (status == FVM_OK)
            status = fvm_avm1_string(c, roots.values[FVM_STRING_BINARY_LEFT_ROOT], &a);
        if (status != FVM_OK)
            return status;
        size_t first = fvm_string_size(a), last = fvm_string_size(b);
        if (first > FVM_STRING_MAX_STORAGE_SIZE - last - 1)
            return FVM_OUT_OF_MEMORY;
        char *bytes = new (std::nothrow) char[first + last + 1];
        if (!bytes)
            return FVM_OUT_OF_MEMORY;
        std::memcpy(bytes, a.string, first);
        std::memcpy(bytes + first, b.string, last);
        bytes[first + last] = 0;
        try
        {
            /* [GC:] Adopt exact bytes atomically without another coercion/depth check. */
            fvm_context_adopt_string(c, bytes, first + last);
        }
        catch (const std::bad_alloc &)
        {
            delete[] bytes;
            return FVM_OUT_OF_MEMORY;
        }
        FVM_Value result;
        status = fvm_value_string_bytes(&result, bytes, first + last);
        if (status == FVM_OK)
            *out = result;
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
