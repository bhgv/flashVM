/* FlashVM: bounded AS1/AS2 percent codecs, preserving counted NUL and lone UTF-16 surrogates. */
#include "url_encoding.hpp"
#include "url_constants.h"
#include "../core/runtime.hpp"
#include "../core/string_internal.hpp"
#include "../core/execution_work_scope.hpp"
#include "../avm1/coercion.hpp"
#include <cstring>
#include <new>

namespace
{


/* AS1/AS2 escape differs from ECMAScript escape: even punctuation such as dot is encoded. */
bool alphanumeric(unsigned char value)
{
    return (value >= '0' && value <= '9') || (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
}


/* Accept either hexadecimal case without locale-dependent ctype calls. */
int hexadecimal(unsigned char value)
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f')
    {
        return value - 'a' + FVM_URL_HEX_ALPHA_FIRST;
    }
    if (value >= 'A' && value <= 'F')
    {
        return value - 'A' + FVM_URL_HEX_ALPHA_FIRST;
    }
    return -1;
}


/* [GC:] Transfer one completed buffer to context storage, freeing it if ownership adoption fails. */
FVM_Result publish(FVM_Context *context, const fvm_detail::vector<char> &bytes, FVM_Value *out)
{
    char *copy = new char[bytes.size() + 1];
    if (!bytes.empty())
    {
        std::memcpy(copy, bytes.begin(), bytes.size());
    }
    copy[bytes.size()] = 0;
    try
    {
        fvm_context_adopt_string(context, copy, bytes.size());
    }
    catch (...)
    {
        delete[] copy;
        throw;
    }
    /* All bytes were constructed as ASCII or validated as counted WTF-8 before adoption. */
    fvm_value_undefined(out);
    out->type = FVM_VALUE_STRING;
    out->string = copy;
    out->string_size = bytes.size();
    return FVM_OK;
}


/* Share coercion, traversal and nested script work without exposing an unfinished result. */
FVM_Result convert(FVM_Context *context, unsigned int count, const FVM_Value *args, FVM_Value *out, bool decode)
{
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(out);
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing || (count && !args))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (context->swf_loaded && context->swf_info.version < FVM_URL_MIN_SWF_VERSION)
    {
        return FVM_UNSUPPORTED;
    }
    if (!count)
    {
        return FVM_OK;
    }


    FVM_ExecutionWorkScope work(context);
    FVM_Value text = args[0];
    /* Counted strings already have their canonical spelling; avoid an unnecessary owned copy. */
    FVM_Result result = text.type == FVM_VALUE_STRING ? FVM_OK : fvm_avm1_string(context, args[0], &text);
    if (result != FVM_OK)
    {
        return result;
    }
    const size_t length = fvm_string_size(text);
    if (length > ((size_t)-1 - 1) / FVM_URL_MAX_EXPANSION)
    {
        return FVM_OUT_OF_MEMORY;
    }
    fvm_detail::vector<char> bytes;
    const char digits[] = "0123456789ABCDEF";
    for (size_t i = 0; i < length; ++i)
    {
        result = work.step();
        if (result != FVM_OK)
        {
            return result;
        }
        unsigned char value = static_cast<unsigned char>(text.string[i]);
        if (decode)
        {
            if (value == '%')
            { /* Reject unsupported malformed spellings instead of inventing reference behavior. */
                if (length - i < FVM_URL_ESCAPE_BYTES)
                {
                    return FVM_UNSUPPORTED;
                }
                int high = hexadecimal(static_cast<unsigned char>(text.string[i + 1]));
                int low = hexadecimal(static_cast<unsigned char>(text.string[i + 2]));
                if (high < 0 || low < 0)
                {
                    return FVM_UNSUPPORTED;
                }
                for (unsigned int charged = 1; charged < FVM_URL_ESCAPE_BYTES; ++charged)
                {
                    result = work.step();
                    if (result != FVM_OK)
                    {
                        return result;
                    }
                }
                value = static_cast<unsigned char>(high * FVM_URL_HEX_RADIX + low);
                i += FVM_URL_ESCAPE_BYTES - 1;
            }
            else if (value == '+')
            {
                value = ' ';
            }
            bytes.push_back(static_cast<char>(value));
        }
        else if (alphanumeric(value))
        {
            bytes.push_back(static_cast<char>(value));
        }
        else
        {
            bytes.push_back('%');
            bytes.push_back(digits[value / FVM_URL_HEX_RADIX]);
            bytes.push_back(digits[value % FVM_URL_HEX_RADIX]);
        }
    }
    if (decode)
    { /* Validate decoded bytes without replacing malformed sequences or dropping embedded NUL. */
        size_t at = 0;
        while (at < bytes.size())
        {
            result = work.step();
            if (result != FVM_OK)
            {
                return result;
            }
            unsigned long cp = 0;
            if (fvm_string_read(bytes.begin(), bytes.size(), at, cp) != FVM_OK)
            {
                return FVM_UNSUPPORTED;
            }
        }
    }
    return publish(context, bytes, out);
}
} // namespace


/* [GC:] Trusted dispatch retains incoming objects while ToString may execute getters and collect. */
FVM_Result fvm_builtin_escape(
    FVM_Context *context, FVM_Object *, unsigned int count, const FVM_Value *args, FVM_Value *out, void *)
{
    return convert(context, count, args, out, false);
}


/* [GC:] Decoding uses the same rooted coercion boundary; errors publish no partially owned text. */
FVM_Result fvm_builtin_unescape(
    FVM_Context *context, FVM_Object *, unsigned int count, const FVM_Value *args, FVM_Value *out, void *)
{
    return convert(context, count, args, out, true);
}
