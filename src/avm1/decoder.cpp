/* FlashVM: AVM1 bytecode decoding and execution: decoder.
 * File: src/avm1/decoder.cpp.
 */

#include "function_record.hpp"
#include "block.hpp"
#include "flashvm/avm1.h"
#include "try_record.hpp"
namespace
{
/* Compute and return inspect. */
FVM_Result inspect(const unsigned char *p, size_t size, FVM_AVM1Info *info, bool block,
                   unsigned int depth)
{
    if (depth > 64)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    size_t pos = 0;
    bool ended = false;
    while (pos < size)
    {
        unsigned int op = p[pos++];
        ++info->instruction_count;
        if (!op)
        {
            ended = true;
            break;
        }
        if (op >= 0x80)
        {
            if (size - pos < 2)
                return FVM_INVALID_ARGUMENT;
            size_t len = (size_t)p[pos] | ((size_t)p[pos + 1] << 8);
            pos += 2;
            if (len > size - pos)
                return FVM_INVALID_ARGUMENT;
            if (op == 0x96)
                ++info->push_count;
            if (op == 0x99 || op == 0x9d)
            {
                if (len != 2)
                    return FVM_INVALID_ARGUMENT;
                ++info->branch_count;
            }
            if (op == 0x8f)
            {
                FVM_TryRecord record;
                if (!fvm_try_record(p, size, pos, len, record))
                    return FVM_INVALID_ARGUMENT;
                pos = record.body;
                for (unsigned int i = 0; i < 3; ++i)
                {
                    FVM_Result result = inspect(p + pos, record.sizes[i], info, true, depth + 1);
                    if (result != FVM_OK)
                        return result;
                    pos += record.sizes[i];
                }
            }
            else if (op == 0x9b || op == 0x8e)
            {
                FVM_FunctionRecord record;
                if (!fvm_function_record(p, size, pos, len, record, op == 0x8e))
                    return FVM_INVALID_ARGUMENT;
                FVM_Result result = inspect(p + record.body, record.size, info, true, depth + 1);
                if (result != FVM_OK)
                    return result;
                pos = record.next;
            }
            else
                pos += len;
        }
    }
    return pos == size && (ended || block) ? FVM_OK : FVM_INVALID_ARGUMENT;
}
} // namespace
/* Compute and return avm1 inspect. */
extern "C" FVM_Result fvm_avm1_inspect(const void *data, size_t size, FVM_AVM1Info *info)
{
    if (!info || info->struct_size != sizeof(*info) || (!data && size))
        return FVM_INVALID_ARGUMENT;
    info->instruction_count = info->push_count = info->branch_count = 0;
    return inspect((const unsigned char *)data, size, info, false, 0);
}

/* Inspect a length-delimited action block without requiring a synthetic input byte. */
FVM_Result fvm_avm1_inspect_block(const void *data, size_t size, FVM_AVM1Info *info)
{
    if (!info || info->struct_size != sizeof(*info) || (!data && size))
        return FVM_INVALID_ARGUMENT;
    info->instruction_count = info->push_count = info->branch_count = 0;
    return inspect((const unsigned char *)data, size, info, true, 0);
}
