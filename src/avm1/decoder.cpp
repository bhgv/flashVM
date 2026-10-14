/* FlashVM: AVM1 bytecode decoding and execution: decoder.
 * File: src/avm1/decoder.cpp.
 */

#include "function_record.hpp"
#include "action_codes.h"
#include "encoding_constants.h"
#include "block.hpp"
#include "flashvm/avm1.h"
#include "try_record.hpp"
#include "with_record.hpp"
namespace
{
/* Compute and return inspect. */
FVM_Result inspect(const unsigned char *p, size_t size, FVM_AVM1Info *info, bool block,
                   unsigned int depth, bool loaded_semantics)
{
    if (depth > FVM_AVM1_MAX_BLOCK_DEPTH)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    size_t pos = 0;
    bool ended = false;
    while (pos < size)
    {
        unsigned int op = p[pos++];
        ++info->instruction_count;
        if (op == FVM_AVM1_ACTION_END)
        {
            ended = true;
            break;
        }
        if (op >= FVM_AVM1_LONG_ACTION_BASE)
        {
            if (size - pos < 2)
                return FVM_INVALID_ARGUMENT;
            size_t len = (size_t)p[pos] | ((size_t)p[pos + 1] << 8);
            pos += 2;
            if (len > size - pos)
                return FVM_INVALID_ARGUMENT;
            if (op == FVM_AVM1_ACTION_PUSH)
                ++info->push_count;
            if (op == FVM_AVM1_ACTION_JUMP || op == FVM_AVM1_ACTION_IF)
            {
                if (len != 2)
                    return FVM_INVALID_ARGUMENT;
                ++info->branch_count;
            }
            if (op == FVM_AVM1_ACTION_WITH)
            {
                size_t body = 0, count = 0, next = 0;
                if (!fvm_with_record(p, size, pos, len, body, count, next))
                    return FVM_INVALID_ARGUMENT;
                FVM_Result result = inspect(p + body, count, info, true, depth + 1, loaded_semantics);
                if (result != FVM_OK) return result;
                pos = next;
            }
            else if (op == FVM_AVM1_ACTION_TRY)
            {
                FVM_TryRecord record;
                if (!fvm_try_record(p, size, pos, len, record))
                    return FVM_INVALID_ARGUMENT;
                pos = record.body;
                for (unsigned int i = 0; i < 3; ++i)
                {
                    FVM_Result result = inspect(p + pos, record.sizes[i], info, true, depth + 1, loaded_semantics);
                    if (result != FVM_OK)
                        return result;
                    pos += record.sizes[i];
                }
            }
            else if (op == FVM_AVM1_ACTION_DEFINE_FUNCTION ||
                     op == FVM_AVM1_ACTION_DEFINE_FUNCTION2)
            {
                FVM_FunctionRecord record;
                if (!fvm_function_record(p, size, pos, len, record,
                                         op == FVM_AVM1_ACTION_DEFINE_FUNCTION2, loaded_semantics))
                    return FVM_INVALID_ARGUMENT;
                FVM_Result result = inspect(p + record.body, record.size, info, true, depth + 1, loaded_semantics);
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
    return inspect((const unsigned char *)data, size, info, false, 0, false);
}

/* Inspect a length-delimited action block without requiring a synthetic input byte. */
FVM_Result fvm_avm1_inspect_block(const void *data, size_t size, FVM_AVM1Info *info)
{
    if (!info || info->struct_size != sizeof(*info) || (!data && size))
        return FVM_INVALID_ARGUMENT;
    info->instruction_count = info->push_count = info->branch_count = 0;
    return inspect((const unsigned char *)data, size, info, true, 0, true);
}

/* Inspect a terminated loaded function while permitting absent register destinations. */
FVM_Result fvm_avm1_inspect_loaded(const void *data, size_t size, FVM_AVM1Info *info)
{
    if (!info || info->struct_size != sizeof(*info) || (!data && size))
        return FVM_INVALID_ARGUMENT;
    info->instruction_count = info->push_count = info->branch_count = 0;
    return inspect((const unsigned char *)data, size, info, false, 0, true);
}
