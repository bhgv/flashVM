/* FlashVM: native Array/Object initializers preserve versioned counts and operand consumption. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "../avm1/literal_constants.h"
/* [GC:] Keep count and entries rooted while conversion, construction and setters execute. */
FVM_Result fvm_jit_literal(FVM_JitInvocation *s, unsigned int object)
{
    FVM_Context *c = s->context;
    fvm_detail::vector<FVM_Value> &stack = *s->stack;
    FVM_Result status = fvm_avm1_require_value(c->swf_loaded, stack);
    if (status != FVM_OK)
        return status;
    FVM_Value normalized;
    status = fvm_exec_literal_count(c, &stack.back(), &normalized);
    if (status != FVM_OK)
        return status;
    if (normalized.type == FVM_VALUE_UNDEFINED)
    {
        stack.back() = normalized;
        return FVM_OK;
    }
    size_t count = (size_t)normalized.number;
    size_t width = object ? FVM_AVM1_OBJECT_LITERAL_ENTRY_WIDTH
                          : FVM_AVM1_ARRAY_LITERAL_ENTRY_WIDTH;
    size_t available = stack.size() - FVM_AVM1_LITERAL_COUNT_OPERANDS;
    if (!c->swf_loaded && count * width > available)
        return FVM_INVALID_ARGUMENT;
    size_t consumed = count * width;
    if (consumed > available)
        consumed = available;
    FVM_Value result;
    if (object)
    {
        fvm_detail::vector<FVM_ExecutionLiteralEntry> entries(count);
        for (size_t i = 0; i < count; ++i)
        {
            size_t pair = i * FVM_AVM1_OBJECT_LITERAL_ENTRY_WIDTH;
            if (pair < available)
                entries[i].value = stack[stack.size() - FVM_AVM1_OBJECT_LITERAL_FIRST_VALUE - pair];
            else
                fvm_value_undefined(&entries[i].value);
            if (pair + FVM_AVM1_OBJECT_LITERAL_NAME_IN_PAIR < available)
                entries[i].name = stack[stack.size() - FVM_AVM1_OBJECT_LITERAL_FIRST_NAME - pair];
            else
                fvm_value_undefined(&entries[i].name);
        }
        status = fvm_exec_literal_object(c, (unsigned int)count,
                                          entries.empty() ? 0 : entries.begin(), &result);
    }
    else
    {
        fvm_detail::vector<FVM_Value> values(count);
        for (size_t i = 0; i < count; ++i)
            if (i < available)
                values[i] = stack[stack.size() - FVM_AVM1_ARRAY_LITERAL_FIRST_VALUE - i];
            else
                fvm_value_undefined(&values[i]);
        status = fvm_exec_literal_array(c, (unsigned int)count,
                                         values.empty() ? 0 : values.begin(), &result);
    }
    if (status == FVM_OK)
    {
        /* [GC:] Publish before shrinking without allocating after construction releases roots. */
        size_t first = stack.size() - FVM_AVM1_LITERAL_COUNT_OPERANDS - consumed;
        stack[first] = result;
        stack.resize(first + 1);
    }
    return status;
}
