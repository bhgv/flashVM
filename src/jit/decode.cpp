/* FlashVM: bounded AVM1 subset lowering into semantic calls and native control-flow labels. */
#include "internal.hpp"
#include "timeline_constants.h"
#include "url_constants.h"
#include "../avm1/action_codes.h"
#include "../avm1/encoding_constants.h"
#include "../avm1/operand_constants.h"
#include "../avm1/number_decode.hpp"
#include "../core/numeric_constants.h"
namespace
{
/* A frame wait branches by encoded Action count rather than source-byte displacement. */
struct FrameSkip
{
    size_t action; /* Native source-action index containing the wait. */
    size_t count; /* Complete following source records to skip when unavailable. */
    /* Record a source-relative skip before branch targets are resolved. */
    FrameSkip(size_t a = 0, size_t c = 0) : action(a), count(c) {}
};
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
    fvm_detail::vector<FrameSkip> skips;
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
        else if (op == FVM_AVM1_ACTION_NEXT_FRAME || op == FVM_AVM1_ACTION_PREVIOUS_FRAME ||
                 op == FVM_AVM1_ACTION_PLAY || op == FVM_AVM1_ACTION_STOP)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_frame_control>,
                op == FVM_AVM1_ACTION_NEXT_FRAME ? FVM_EXEC_NEXT_FRAME :
                op == FVM_AVM1_ACTION_PREVIOUS_FRAME ? FVM_EXEC_PREVIOUS_FRAME :
                op == FVM_AVM1_ACTION_PLAY ? FVM_EXEC_PLAY : FVM_EXEC_STOP));
        else if (op == FVM_AVM1_ACTION_GOTO_FRAME || op == FVM_AVM1_ACTION_GOTO_LABEL)
        {
            FVM_JitConstant frame;
            if (op == FVM_AVM1_ACTION_GOTO_FRAME)
            {
                if (length != FVM_AVM1_DIRECT_FRAME_BYTES)
                    return FVM_INVALID_ARGUMENT;
                fvm_value_number(&frame.value, word(data + payload) + FVM_AVM1_DIRECT_FRAME_BASE);
            }
            else
            {
                if (!length || data[position - 1] || std::memchr(data + payload, 0, length - 1))
                    return FVM_INVALID_ARGUMENT;
                frame.value.type = FVM_VALUE_STRING;
                frame.text = fvm_detail::string((const char *)data + payload, length - 1);
            }
            if (code.constants.size() >= FVM_JIT_MAX_CONSTANTS)
                return FVM_UNSUPPORTED;
            unsigned int index = (unsigned int)code.constants.size();
            code.constants.push_back(frame);
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_frame_literal>, index));
        }
        else if (op == FVM_AVM1_ACTION_GOTO_FRAME2)
        {
            if ((length != FVM_AVM1_GOTO_FLAGS_BYTES && length != FVM_AVM1_GOTO_SCENE_BYTES) ||
                (data[payload] & ~FVM_AVM1_GOTO_VALID_FLAGS) ||
                length != ((data[payload] & FVM_AVM1_GOTO_SCENE_BIAS)
                    ? FVM_AVM1_GOTO_SCENE_BYTES : FVM_AVM1_GOTO_FLAGS_BYTES))
                return FVM_INVALID_ARGUMENT;
            unsigned int policy = (data[payload] & FVM_AVM1_GOTO_PLAY) ? FVM_JIT_FRAME_PLAY : 0;
            if (data[payload] & FVM_AVM1_GOTO_SCENE_BIAS)
                policy |= FVM_JIT_FRAME_WITH_BIAS |
                    (word(data + payload + FVM_AVM1_GOTO_SCENE_OFFSET) << FVM_JIT_FRAME_BIAS_SHIFT);
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_frame_value>, policy));
        }
        else if (op == FVM_AVM1_ACTION_WAIT_FOR_FRAME || op == FVM_AVM1_ACTION_WAIT_FOR_FRAME2)
        {
            bool direct = op == FVM_AVM1_ACTION_WAIT_FOR_FRAME;
            if (length != (direct ? FVM_AVM1_WAIT_DIRECT_BYTES : FVM_AVM1_WAIT_VALUE_BYTES))
                return FVM_INVALID_ARGUMENT;
            action.calls.push_back(FVM_JitCall(
                direct ? fvm_jit_safe<fvm_jit_wait_direct> : fvm_jit_safe<fvm_jit_wait_value>,
                direct ? word(data + payload) : 0));
            action.branch = FVM_JIT_CONDITIONAL;
            skips.push_back(FrameSkip(code.actions.size(),
                data[payload + (direct ? FVM_AVM1_WAIT_DIRECT_SKIP_OFFSET : 0)]));
        }
        else if (op == FVM_AVM1_ACTION_GET_URL)
        {
            const unsigned char *separator = (const unsigned char *)std::memchr(data + payload, 0, length);
            if (!separator)
                return FVM_INVALID_ARGUMENT;
            size_t first = (size_t)(separator - (data + payload));
            size_t remaining = length - first - 1;
            if (!remaining || data[position - 1] || std::memchr(separator + 1, 0, remaining - 1))
                return FVM_INVALID_ARGUMENT;
            if (code.constants.size() > FVM_JIT_MAX_CONSTANTS - FVM_JIT_URL_LITERAL_CONSTANT_COUNT)
                return FVM_UNSUPPORTED;
            unsigned int index = (unsigned int)code.constants.size();
            FVM_JitConstant url, target;
            url.value.type = target.value.type = FVM_VALUE_STRING;
            url.text = fvm_detail::string((const char *)data + payload, first);
            target.text = fvm_detail::string((const char *)separator + 1, remaining - 1);
            code.constants.push_back(url);
            code.constants.push_back(target);
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_url_literal>, index));
        }
        else if (op == FVM_AVM1_ACTION_GET_URL2)
        {
            if (length != FVM_AVM1_URL_FLAGS_BYTES || (data[payload] & FVM_AVM1_URL_RESERVED_MASK) ||
                (data[payload] & FVM_AVM1_URL_METHOD_MASK) == FVM_AVM1_URL_INVALID_METHOD)
                return FVM_INVALID_ARGUMENT;
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_url_values>, data[payload]));
        }
        else if (op == FVM_AVM1_ACTION_CALL)
        {
            if (length)
                return FVM_INVALID_ARGUMENT;
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_call_frame>, 0));
        }
        else if (op == FVM_AVM1_ACTION_SET_TARGET2)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_target_value>, 0));
        else if (op == FVM_AVM1_ACTION_SET_TARGET)
        {
            size_t length = position - payload;
            if (!length || data[position - 1] || std::memchr(data + payload, 0, length - 1))
                return FVM_INVALID_ARGUMENT;
            if (code.constants.size() >= FVM_JIT_MAX_CONSTANTS)
                return FVM_UNSUPPORTED;
            FVM_JitConstant path;
            path.value.type = FVM_VALUE_STRING;
            path.text = fvm_detail::string((const char *)data + payload, length - 1);
            unsigned int index = (unsigned int)code.constants.size();
            code.constants.push_back(path);
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_target_literal>, index));
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
        else if (op == FVM_AVM1_ACTION_STRING_LENGTH || op == FVM_AVM1_ACTION_MB_STRING_LENGTH)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_string_length>, 0));
        else if (op == FVM_AVM1_ACTION_CHAR_TO_ASCII || op == FVM_AVM1_ACTION_MB_CHAR_TO_ASCII)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_char_code>, 0));
        else if (op == FVM_AVM1_ACTION_ASCII_TO_CHAR || op == FVM_AVM1_ACTION_MB_ASCII_TO_CHAR)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_char_from_code>, 0));
        else if (op == FVM_AVM1_ACTION_STRING_EXTRACT || op == FVM_AVM1_ACTION_MB_STRING_EXTRACT)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_string_extract>, 0));
        else if (op == FVM_AVM1_ACTION_STRING_ADD || op == FVM_AVM1_ACTION_STRING_EQUALS ||
                 op == FVM_AVM1_ACTION_STRING_LESS || op == FVM_AVM1_ACTION_STRING_GREATER)
        {
            unsigned int operation = op == FVM_AVM1_ACTION_STRING_ADD ? FVM_EXEC_ADD_DYNAMIC
                : op == FVM_AVM1_ACTION_STRING_EQUALS ? FVM_EXEC_EQUAL
                : op == FVM_AVM1_ACTION_STRING_LESS ? FVM_EXEC_LESS : FVM_EXEC_GREATER;
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_string_binary>, operation));
        }
        else if (op == FVM_AVM1_ACTION_INIT_ARRAY || op == FVM_AVM1_ACTION_INIT_OBJECT)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_literal>,
                                               op == FVM_AVM1_ACTION_INIT_OBJECT));
        else if (op == FVM_AVM1_ACTION_EQUALS || op == FVM_AVM1_ACTION_LESS)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_legacy_compare>,
                op == FVM_AVM1_ACTION_EQUALS ? FVM_EXEC_LEGACY_EQUAL : FVM_EXEC_LEGACY_LESS));
        else if (op == FVM_AVM1_ACTION_AND || op == FVM_AVM1_ACTION_OR)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_logical>,
                op == FVM_AVM1_ACTION_AND ? FVM_EXEC_LOGICAL_AND : FVM_EXEC_LOGICAL_OR));
        else if (op == FVM_AVM1_ACTION_EXTENDS)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_extends>, 0));
        else if (op == FVM_AVM1_ACTION_IMPLEMENTS)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_implements>, 0));
        else if (op == FVM_AVM1_ACTION_CAST || op == FVM_AVM1_ACTION_INSTANCE_OF)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_class_match>,
                                               op == FVM_AVM1_ACTION_CAST));
        else if (op == FVM_AVM1_ACTION_GET_PROPERTY || op == FVM_AVM1_ACTION_SET_PROPERTY)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_display_property>,
                                               op == FVM_AVM1_ACTION_SET_PROPERTY));
        else if (op == FVM_AVM1_ACTION_TARGET_PATH)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_target_path>, 0));
        else if (op == FVM_AVM1_ACTION_ENUMERATE || op == FVM_AVM1_ACTION_ENUMERATE2)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_enumerate>,
                                               op == FVM_AVM1_ACTION_ENUMERATE));
        else if (op == FVM_AVM1_ACTION_CLONE_SPRITE)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_clone_sprite>, 0));
        else if (op == FVM_AVM1_ACTION_REMOVE_SPRITE)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_remove_sprite>, 0));
        else if (op == FVM_AVM1_ACTION_START_DRAG)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_start_drag>, 0));
        else if (op == FVM_AVM1_ACTION_END_DRAG)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_end_drag>, 0));
        else if (op == FVM_AVM1_ACTION_TRACE)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_trace>, 0));
        else if (op == FVM_AVM1_ACTION_THROW)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_throw>, 0));
        else if (op == FVM_AVM1_ACTION_GET_TIME)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_get_time>, 0));
        else if (op == FVM_AVM1_ACTION_RANDOM_NUMBER)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_random_number>, 0));
        else if (op == FVM_AVM1_ACTION_GET_VARIABLE)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_get_variable>, 0));
        else if (op == FVM_AVM1_ACTION_SET_VARIABLE)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_set_variable>, 0));
        else if (op == FVM_AVM1_ACTION_DEFINE_LOCAL || op == FVM_AVM1_ACTION_DEFINE_LOCAL2)
            action.calls.push_back(FVM_JitCall(
                fvm_jit_safe<fvm_jit_declare_variable>, op == FVM_AVM1_ACTION_DEFINE_LOCAL
                    ? FVM_EXEC_DECLARE_INITIALIZED : FVM_EXEC_DECLARE_UNINITIALIZED));
        else if (op == FVM_AVM1_ACTION_CALL_FUNCTION || op == FVM_AVM1_ACTION_NEW_OBJECT)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_call_named>,
                op == FVM_AVM1_ACTION_NEW_OBJECT));
        else if (op == FVM_AVM1_ACTION_CALL_METHOD || op == FVM_AVM1_ACTION_NEW_METHOD)
            action.calls.push_back(FVM_JitCall(fvm_jit_safe<fvm_jit_call_method>,
                op == FVM_AVM1_ACTION_NEW_METHOD));
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
    size_t counted_actions = code.actions.size();
    size_t skip_at = 0;
    for (size_t i = 0; i < code.actions.size(); ++i)
    {
        if (skip_at < skips.size() && skips[skip_at].action == i)
        {
            ++skip_at;
            continue;
        }
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
    }
    /* Early exits skip even ActionEnd; synthetic epilogues must not charge source checkpoints. */
    size_t early_exit = code.actions.size();
    bool have_early_exit = false;
    for (size_t i = 0; i < skips.size(); ++i)
    {
        size_t at = skips[i].action;
        size_t target = 0;
        if (skips[i].count >= counted_actions - at - 1)
        {
            if (!have_early_exit)
            {
                if (code.actions.size() >= FVM_JIT_MAX_ACTIONS)
                    return FVM_UNSUPPORTED;
                FVM_JitAction terminal;
                terminal.offset = size;
                terminal.branch = FVM_JIT_EXIT;
                terminal.calls.push_back(FVM_JitCall(fvm_jit_finish, 0));
                code.actions.push_back(terminal);
                have_early_exit = true;
            }
            target = early_exit;
        }
        else
            target = at + 1 + skips[i].count;
        code.actions[at].target = target;
    }
    return FVM_OK;
}
