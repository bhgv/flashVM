/* FlashVM: bounded AVM1 subset lowering into semantic calls and native control-flow labels. */
#include "internal.hpp"
#include "../avm1/action_codes.h"
#include "../avm1/encoding_constants.h"
#include "../avm1/operand_constants.h"
#include "../avm1/number_decode.hpp"
#include "../core/numeric_constants.h"
namespace
{
/* Read the bounded little-endian branch/record width. */
unsigned int word(const unsigned char *p)
{
    return p[0] | ((unsigned int)p[1] << FVM_JIT_BYTE_BITS);
}
/* Read an integer without alignment, host endian or signed-cast assumptions. */
unsigned long dword(const unsigned char *p)
{
    unsigned long value = 0;
    for (unsigned int i = 0; i < FVM_AVM1_DWORD_BYTES; ++i)
        value |= (unsigned long)p[i] << (FVM_JIT_BYTE_BITS * i);
    return value;
}
/* Snapshot literal bytes; no context-managed value enters the immutable code handle. */
FVM_Result push(const unsigned char *data, size_t begin, size_t end, FVM_JitCode &code,
                FVM_JitAction &action)
{
    while (begin < end)
    {
        unsigned int type = data[begin++];
        if (type == FVM_AVM1_PUSH_REGISTER)
        {
            if (begin == end)
                return FVM_INVALID_ARGUMENT;
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_load_register>, data[begin++]));
            continue;
        }
        if (type == FVM_AVM1_PUSH_CONSTANT8 || type == FVM_AVM1_PUSH_CONSTANT16)
        {
            size_t width = type == FVM_AVM1_PUSH_CONSTANT8 ? FVM_AVM1_CONSTANT8_INDEX_BYTES
                                                          : FVM_AVM1_CONSTANT16_INDEX_BYTES;
            if (end - begin < width)
                return FVM_INVALID_ARGUMENT;
            unsigned int index = type == FVM_AVM1_PUSH_CONSTANT8 ? data[begin] : word(data + begin);
            begin += width;
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_push_pool>, index));
            continue;
        }
        FVM_JitConstant constant;
        if (type == FVM_AVM1_PUSH_INTEGER)
        {
            if (end - begin < FVM_AVM1_DWORD_BYTES)
                return FVM_INVALID_ARGUMENT;
            unsigned long bits = dword(data + begin);
            fvm_value_number(&constant.value, bits & FVM_UINT32_SIGN_BIT
                                                  ? (double)bits - FVM_UINT32_MODULUS
                                                  : (double)bits);
            begin += FVM_AVM1_DWORD_BYTES;
        }
        else if (type == FVM_AVM1_PUSH_FLOAT)
        {
            if (end - begin < FVM_AVM1_DWORD_BYTES)
                return FVM_INVALID_ARGUMENT;
            fvm_value_number(&constant.value, fvm_avm1_decode_float(dword(data + begin)));
            begin += FVM_AVM1_DWORD_BYTES;
        }
        else if (type == FVM_AVM1_PUSH_DOUBLE)
        {
            if (end - begin < FVM_AVM1_DOUBLE_BYTES)
                return FVM_INVALID_ARGUMENT;
            fvm_value_number(&constant.value, fvm_avm1_decode_double(
                dword(data + begin), dword(data + begin + FVM_AVM1_DWORD_BYTES)));
            begin += FVM_AVM1_DOUBLE_BYTES;
        }
        else if (type == FVM_AVM1_PUSH_STRING)
        {
            size_t first = begin;
            while (begin < end && data[begin])
                ++begin;
            if (begin == end)
                return FVM_INVALID_ARGUMENT;
            constant.text = fvm_detail::string((const char *)data + first, begin - first);
            constant.value.type = FVM_VALUE_STRING;
            ++begin;
        }
        else if (type == FVM_AVM1_PUSH_BOOLEAN)
        {
            if (begin == end)
                return FVM_INVALID_ARGUMENT;
            fvm_value_boolean(&constant.value, data[begin++] != 0);
        }
        else if (type == FVM_AVM1_PUSH_NULL)
            constant.value.type = FVM_VALUE_NULL;
        else if (type != FVM_AVM1_PUSH_UNDEFINED)
            return FVM_UNSUPPORTED;
        if (code.constants.size() >= FVM_JIT_MAX_CONSTANTS)
            return FVM_UNSUPPORTED;
        unsigned int index = (unsigned int)code.constants.size();
        code.constants.push_back(constant);
        action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_push>, index));
    }
    return FVM_OK;
}
/* Snapshot each complete replacement; choosing which pool is active belongs to native execution. */
FVM_Result pool(const unsigned char *data, size_t begin, size_t end, FVM_JitCode &code,
                FVM_JitAction &action)
{
    if (end - begin < FVM_AVM1_POOL_COUNT_BYTES)
        return FVM_INVALID_ARGUMENT;
    unsigned int count = word(data + begin);
    begin += FVM_AVM1_POOL_COUNT_BYTES;
    if (count > FVM_JIT_MAX_POOL_ENTRIES - code.pool_entries)
        return FVM_UNSUPPORTED;
    FVM_JitPool replacement;
    for (unsigned int i = 0; i < count; ++i)
    {
        size_t first = begin;
        while (begin < end && data[begin])
            ++begin;
        if (begin == end)
            return FVM_INVALID_ARGUMENT;
        replacement.entries.push_back(fvm_detail::string((const char *)data + first, begin - first));
        ++begin;
    }
    if (begin != end)
        return FVM_INVALID_ARGUMENT;
    unsigned int index = (unsigned int)code.pools.size();
    code.pools.push_back(replacement);
    code.pool_entries += count;
    action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_set_pool>, index));
    return FVM_OK;
}
} // namespace
/* Reject unsupported bodies atomically; retain byte offsets only as debugger/relocation metadata.
 */
FVM_Result fvm_jit_decode(const unsigned char *data, size_t size, FVM_JitCode &code)
{
    size_t position = 0;
    bool ended = false;
    while (position < size)
    {
        if (code.actions.size() >= FVM_JIT_MAX_ACTIONS)
            return FVM_UNSUPPORTED;
        FVM_JitAction action;
        action.offset = position;
        action.calls.push_back(
            FVM_JitCall(fvm_jit_safe<fvm_jit_checkpoint>, (unsigned int)position));
        unsigned int op = data[position++];
        size_t length = 0, payload = position;
        if (op >= FVM_AVM1_LONG_ACTION_BASE)
        {
            if (size - position < FVM_AVM1_BRANCH_BYTES)
                return FVM_INVALID_ARGUMENT;
            length = word(data + position);
            position += FVM_AVM1_BRANCH_BYTES;
            payload = position;
            if (length > size - position)
                return FVM_INVALID_ARGUMENT;
            position += length;
        }
        FVM_ExecutionNumeric numeric = FVM_EXEC_ADD;
        bool is_numeric = true;
        switch (op)
        {
        case FVM_AVM1_ACTION_ADD:
            numeric = FVM_EXEC_ADD;
            break;
        case FVM_AVM1_ACTION_SUBTRACT:
            numeric = FVM_EXEC_SUBTRACT;
            break;
        case FVM_AVM1_ACTION_MULTIPLY:
            numeric = FVM_EXEC_MULTIPLY;
            break;
        case FVM_AVM1_ACTION_DIVIDE:
            numeric = FVM_EXEC_DIVIDE;
            break;
        case FVM_AVM1_ACTION_MODULO:
            numeric = FVM_EXEC_REMAINDER;
            break;
        case FVM_AVM1_ACTION_BIT_AND:
            numeric = FVM_EXEC_BIT_AND;
            break;
        case FVM_AVM1_ACTION_BIT_OR:
            numeric = FVM_EXEC_BIT_OR;
            break;
        case FVM_AVM1_ACTION_BIT_XOR:
            numeric = FVM_EXEC_BIT_XOR;
            break;
        case FVM_AVM1_ACTION_BIT_LSHIFT:
            numeric = FVM_EXEC_SHIFT_LEFT;
            break;
        case FVM_AVM1_ACTION_BIT_RSHIFT:
            numeric = FVM_EXEC_SHIFT_RIGHT;
            break;
        case FVM_AVM1_ACTION_BIT_URSHIFT:
            numeric = FVM_EXEC_SHIFT_UNSIGNED;
            break;
        default:
            is_numeric = false;
        }
        if (is_numeric)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_numeric>, numeric));
        else if (op == FVM_AVM1_ACTION_PUSH)
        {
            FVM_Result status = push(data, payload, position, code, action);
            if (status != FVM_OK)
                return status;
        }
        else if (op == FVM_AVM1_ACTION_CONSTANT_POOL)
        {
            FVM_Result status = pool(data, payload, position, code, action);
            if (status != FVM_OK)
                return status;
        }
        else if (op == FVM_AVM1_ACTION_END)
        {
            action.branch = FVM_JIT_EXIT;
            ended = true;
        }
        else if (op == FVM_AVM1_ACTION_RETURN)
        {
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_return>, 0));
            action.branch = FVM_JIT_EXIT;
        }
        else if (op == FVM_AVM1_ACTION_TO_INTEGER || op == FVM_AVM1_ACTION_TO_NUMBER ||
                 op == FVM_AVM1_ACTION_TO_STRING || op == FVM_AVM1_ACTION_TYPE_OF ||
                 op == FVM_AVM1_ACTION_INCREMENT || op == FVM_AVM1_ACTION_DECREMENT ||
                 op == FVM_AVM1_ACTION_NOT)
        {
            unsigned int operation = op == FVM_AVM1_ACTION_TO_INTEGER ? FVM_JIT_UNARY_INTEGER
                : op == FVM_AVM1_ACTION_TO_NUMBER ? FVM_JIT_UNARY_NUMBER
                : op == FVM_AVM1_ACTION_TO_STRING ? FVM_JIT_UNARY_STRING
                : op == FVM_AVM1_ACTION_TYPE_OF ? FVM_JIT_UNARY_TYPE
                : op == FVM_AVM1_ACTION_INCREMENT ? FVM_JIT_UNARY_INCREMENT
                : op == FVM_AVM1_ACTION_DECREMENT ? FVM_JIT_UNARY_DECREMENT
                                                 : FVM_JIT_UNARY_NOT;
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_unary>, operation));
        }
        else if (op == FVM_AVM1_ACTION_STORE_REGISTER)
        {
            if (length != FVM_AVM1_STORE_REGISTER_BYTES)
                return FVM_INVALID_ARGUMENT;
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_store_register>, data[payload]));
        }
        else if (op == FVM_AVM1_ACTION_GET_VARIABLE)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_get_variable>, 0));
        else if (op == FVM_AVM1_ACTION_SET_VARIABLE)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_set_variable>, 0));
        else if (op == FVM_AVM1_ACTION_DEFINE_LOCAL || op == FVM_AVM1_ACTION_DEFINE_LOCAL2)
            action.calls.push_back(FVM_JitCall(
                fvm_jit_safe<fvm_jit_declare_variable>, op == FVM_AVM1_ACTION_DEFINE_LOCAL
                    ? FVM_EXEC_DECLARE_INITIALIZED : FVM_EXEC_DECLARE_UNINITIALIZED));
        else if (op == FVM_AVM1_ACTION_GET_MEMBER || op == FVM_AVM1_ACTION_SET_MEMBER)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_member>,
                op == FVM_AVM1_ACTION_SET_MEMBER ? FVM_JIT_MEMBER_WRITE : FVM_JIT_MEMBER_READ));
        else if (op == FVM_AVM1_ACTION_DELETE)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_delete_member>, 0));
        else if (op == FVM_AVM1_ACTION_DELETE2)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_delete_reference>, 0));
        else if (op == FVM_AVM1_ACTION_ADD2 || op == FVM_AVM1_ACTION_LESS2 ||
                 op == FVM_AVM1_ACTION_EQUALS2 || op == FVM_AVM1_ACTION_STRICT_EQUALS ||
                 op == FVM_AVM1_ACTION_GREATER)
        {
            unsigned int operation = op == FVM_AVM1_ACTION_ADD2            ? FVM_EXEC_ADD_DYNAMIC
                                     : op == FVM_AVM1_ACTION_LESS2         ? FVM_EXEC_LESS
                                     : op == FVM_AVM1_ACTION_EQUALS2       ? FVM_EXEC_EQUAL
                                     : op == FVM_AVM1_ACTION_STRICT_EQUALS ? FVM_EXEC_STRICT_EQUAL
                                                                           : FVM_EXEC_GREATER;
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_compare>, operation));
        }
        else if (op == FVM_AVM1_ACTION_POP || op == FVM_AVM1_ACTION_PUSH_DUPLICATE ||
                 op == FVM_AVM1_ACTION_STACK_SWAP)
            action.calls.push_back(
                FVM_JitCall(fvm_jit_safe<fvm_jit_stack>, op == FVM_AVM1_ACTION_POP ? FVM_JIT_POP
                                                         : op == FVM_AVM1_ACTION_PUSH_DUPLICATE
                                                             ? FVM_JIT_DUPLICATE
                                                             : FVM_JIT_SWAP));
        else if (op == FVM_AVM1_ACTION_JUMP || op == FVM_AVM1_ACTION_IF)
        {
            if (length != FVM_AVM1_BRANCH_BYTES)
                return FVM_INVALID_ARGUMENT;
            long delta = word(data + payload);
            if (delta >= FVM_AVM1_BRANCH_SIGN_BIT)
                delta -= FVM_AVM1_BRANCH_MODULUS;
            if ((delta < 0 && (size_t)-delta > position) ||
                (delta >= 0 && (size_t)delta > size - position))
                return FVM_INVALID_ARGUMENT;
            action.target = delta < 0 ? position - (size_t)-delta : position + (size_t)delta;
            action.branch = op == FVM_AVM1_ACTION_IF ? FVM_JIT_CONDITIONAL : FVM_JIT_JUMP;
            if (action.branch == FVM_JIT_CONDITIONAL)
                action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_condition>, 0));
        }
        else
            return FVM_UNSUPPORTED;
        code.actions.push_back(action);
        if (ended)
            break;
    }
    if (!ended)
    {
        if (code.actions.size() >= FVM_JIT_MAX_ACTIONS)
            return FVM_UNSUPPORTED;
        FVM_JitAction terminal;
        terminal.offset = size;
        terminal.branch = FVM_JIT_EXIT;
        terminal.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_checkpoint>, (unsigned int)size));
        code.actions.push_back(terminal);
    }
    for (size_t i = 0; i < code.actions.size(); ++i)
        if (code.actions[i].branch == FVM_JIT_JUMP || code.actions[i].branch == FVM_JIT_CONDITIONAL)
        {
            size_t low = 0, high = code.actions.size();
            while (low < high)
            {
                size_t middle = low + (high - low) / 2;
                if (code.actions[middle].offset < code.actions[i].target)
                    low = middle + 1;
                else
                    high = middle;
            }
            if (low == code.actions.size() || code.actions[low].offset != code.actions[i].target)
                return FVM_INVALID_ARGUMENT;
            code.actions[i].target = low;
        }
    return FVM_OK;
}
