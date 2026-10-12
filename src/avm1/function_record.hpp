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
    size_t name /* Name used to identify or look up this entry. */,
        name_size /* Byte count of the function name. */,
        parameters /* Formal parameter names in declaration order. */,
        body /* Bytecode storage or view for this record. */,
        size /* Size of the associated payload or collection. */,
        next;           /* Link to the next entry. */
    unsigned int count; /* Number of active entries. */
};
/* Standard DefineFunction puts its code after the action header. Accept the
 * historical inline encoding as a compatibility input, but bound both forms. */
/* Compute and return function record. */
inline bool fvm_function_record(const unsigned char *data, size_t size, size_t start, size_t length,
                                FVM_FunctionRecord &r)
{
    if (start > size || length > size - start)
        return false;
    size_t p = start, end = start + length;
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
    r.parameters = p;
    for (unsigned int i = 0; i < r.count; ++i)
    {
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
