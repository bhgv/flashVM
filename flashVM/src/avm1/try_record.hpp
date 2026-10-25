/* FlashVM: AVM1 bytecode decoding and execution: try record.
 * File: src/avm1/try_record.hpp.
 */

#ifndef FVM_TRY_RECORD_HPP
/* Define FVM TRY RECORD HPP constant or compilation switch. */
#define FVM_TRY_RECORD_HPP
#include "encoding_constants.h"
#include <cstddef>


/* ActionTry's length covers its header only. The three action blocks follow
 * outside the record length, in try/catch/finally order. */
/* Define FVM_TryRecord struct and its representation. */
struct FVM_TryRecord
{
    unsigned int flags /* Bit flags controlling the operation. */,
        reg;          /* Register index used for a caught exception. */
    size_t sizes[FVM_AVM1_TRY_BLOCK_COUNT] /* Byte lengths of the try, catch and finally blocks. */,
        body;         /* Bytecode storage or view for this record. */
    const char *name; /* Name used to identify or look up this entry. */
};


/* Attempt record. */
static bool fvm_try_record(const unsigned char *data, size_t size, size_t payload, size_t length, FVM_TryRecord &out)
{
    if (length < 8 || payload > size || length > size - payload)
    {
        return false;
    }
    const unsigned char *p = data + payload;

    /* Computation / Вычисление и обновление состояния. */
    out.flags = p[0];
    out.reg = 0;
    out.name = 0;
    if (out.flags & ~FVM_AVM1_TRY_VALID_FLAGS)
    {
        return false;
    }
    size_t total = 0;
    for (unsigned int i = 0; i < 3; ++i)
    {
        out.sizes[i] = (size_t)p[1 + i * 2] | ((size_t)p[2 + i * 2] << 8);
        total += out.sizes[i];
    }
    if (!(out.flags & FVM_AVM1_TRY_CATCH) && out.sizes[FVM_AVM1_TRY_CATCH_INDEX])
    {
        return false;
    }
    if (!(out.flags & FVM_AVM1_TRY_FINALLY) && out.sizes[FVM_AVM1_TRY_FINALLY_INDEX])
    {
        return false;
    }
    if (out.flags & FVM_AVM1_TRY_CATCH_REGISTER)
    {
        if (length != 8)
        {
            return false;
        }
        out.reg = p[7];
    }
    else
    {
        size_t i = 7;
        while (i < length && p[i])
        {
            ++i;
        }
        if (i + 1 != length)
        {
            return false;
        }
        out.name = (const char *)p + 7;
    }
    out.body = payload + length;
    return total <= size - out.body;
}
#endif
