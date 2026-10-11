#ifndef FVM_AVM1_FUNCTION_RECORD_HPP
#define FVM_AVM1_FUNCTION_RECORD_HPP
#include <stddef.h>
struct FVM_FunctionRecord
{
    size_t name, name_size, parameters, body, size, next;
    unsigned int count;
};
/* Standard DefineFunction puts its code after the action header. Accept the
 * historical inline encoding as a compatibility input, but bound both forms. */
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
    r.name_size = p - r.name;
    ++p;
    if (end - p < 2)
        return false;
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
