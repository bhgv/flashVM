/* FlashVM: AVM1 bytecode decoding and execution: function record.
 * File: src/avm1/function_record.hpp.
 */

#ifndef FVM_AVM1_FUNCTION_RECORD_HPP
/* Define FVM AVM1 FUNCTION RECORD HPP constant or compilation switch. */
#define FVM_AVM1_FUNCTION_RECORD_HPP
#include <stddef.h>
/* Define FVM_FunctionRecord struct and its representation. */
struct FVM_FunctionRecord
{
    size_t name;                 /* Byte offset of the terminated function name. */
    size_t name_size;            /* Name byte count, excluding the terminator. */
    size_t parameters;           /* Offset of the first parameter record. */
    size_t body;                 /* Offset of the first bytecode instruction. */
    size_t size;                 /* Byte length of the function body. */
    size_t next;                 /* Offset immediately following the body. */
    unsigned int count;          /* Number of formal parameters in the record. */
    unsigned int register_count; /* Number of private registers requested by Function2. */
    unsigned int flags;          /* Preload and implicit-binding suppression bits. */
    bool function2;              /* Whether this record uses the DefineFunction2 layout. */
};
/* Reject reserved bits; preload and suppression may be set simultaneously. */
inline bool fvm_function2_flags_valid(unsigned int flags)
{
    return !(flags & ~511U);
}

/* Standard DefineFunction puts its code after the action header. Accept the
 * historical inline encoding as a compatibility input, but bound both forms. */
/* Compute and return function record. */
inline bool fvm_function_record(const unsigned char *data, size_t size, size_t start, size_t length,
                                FVM_FunctionRecord &r, bool function2 = false)
{
    if (start > size || length > size - start)
        return false;
    size_t p = start, end = start + length;
    r.function2 = function2;
    r.register_count = r.flags = 0;
    r.name = p;
    while (p < end && data[p])
        ++p;
    if (p == end)
        return false;

    /* Computation / Вычисление и обновление состояния. */
    r.name_size = p - r.name;
    ++p;
    if (end - p < 2)
        return false;

    /* Computation / Вычисление и обновление состояния. */
    r.count = data[p] | ((unsigned int)data[p + 1] << 8);
    p += 2;

    if (function2)
    {
        if (end - p < 3)
            return false;
        r.register_count = data[p++];
        r.flags = data[p] | ((unsigned int)data[p + 1] << 8);
        p += 2;
        if (!fvm_function2_flags_valid(r.flags))
            return false;
        unsigned int preload = 0;
        const unsigned int bits[] = {1, 4, 16, 64, 128, 256};
        for (unsigned int i = 0; i < 6; ++i)
            if (r.flags & bits[i])
                ++preload;
        if (preload > r.register_count)
            return false;
    }
    r.parameters = p;
    for (unsigned int i = 0; i < r.count; ++i)
    {

        if (function2)
        {
            if (p == end || data[p] > r.register_count)
                return false;
            ++p;
        }
        while (p < end && data[p])
            ++p;
        if (p == end)
            return false;
        ++p;
    }
    if (end - p < 2)
        return false;

    /* Computation / Вычисление и обновление состояния. */
    r.size = data[p] | ((unsigned int)data[p + 1] << 8);
    p += 2;
    if (p != end && r.size != end - p)
        return false;
    r.body = p;
    if (r.size > size - p)
        return false;
    r.next = p + r.size;
    return true;
}
#endif
