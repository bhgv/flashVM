/* FlashVM: interpreter-only preparation of audited missing AVM1 operands.
 * Value-based execution services keep their explicit input-pointer validation. */
#ifndef FVM_AVM1_OPERAND_STACK_HPP
#define FVM_AVM1_OPERAND_STACK_HPP /* Missing operand preparation guard. */
#include "../core/runtime.hpp"


/* [GC:] Preserve existing top operands while filling missing lower operands with undefined.
 * The caller already roots the vector. Allocation completes before shifting values;
 * no script/collection occurs during relocation. Host-only execution stays strict. */
inline FVM_Result fvm_avm1_require_operands(bool allow_missing, fvm_detail::vector<FVM_Value> &stack, size_t required)
{
    if (stack.size() >= required)
    {
        return FVM_OK;
    }
    if (!allow_missing)
    {
        return FVM_ERROR;
    }
    size_t previous = stack.size(), missing = required - previous;
    stack.resize(required);
    for (size_t i = previous; i > 0; --i)
    {
        stack[missing + i - 1] = stack[i - 1];
    }
    for (size_t i = 0; i < missing; ++i)
    {
        fvm_value_undefined(&stack[i]);
    }
    return FVM_OK;
}


/* [GC:] The caller already roots this operand vector; push undefined before service dispatch.
 * Loaded bytecode follows AVM1's missing-pop policy; host-only execution stays strict.
 * Allocation exceptions propagate to the interpreter's existing containment scope. */
inline FVM_Result fvm_avm1_require_value(bool allow_missing, fvm_detail::vector<FVM_Value> &stack)
{
    if (!stack.empty())
    {
        return FVM_OK;
    }
    if (!allow_missing)
    {
        return FVM_ERROR;
    }
    FVM_Value missing;
    fvm_value_undefined(&missing);
    stack.push_back(missing);
    return FVM_OK;
}
#endif
