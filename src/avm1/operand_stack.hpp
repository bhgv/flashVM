/* FlashVM: interpreter-only preparation of missing unary AVM1 operands.
 * Value-based execution services keep their explicit input-pointer validation. */
#ifndef FVM_AVM1_OPERAND_STACK_HPP
#define FVM_AVM1_OPERAND_STACK_HPP /* Unary operand preparation guard. */
#include "../core/runtime.hpp"
/* [GC:] The caller already roots this operand vector; push undefined before service dispatch.
 * Loaded bytecode follows AVM1's missing-pop policy; host-only execution stays strict.
 * Allocation exceptions propagate to the interpreter's existing containment scope. */
inline FVM_Result fvm_avm1_require_value(bool allow_missing, fvm_detail::vector<FVM_Value> &stack)
{
    if (!stack.empty())
        return FVM_OK;
    if (!allow_missing)
        return FVM_ERROR;
    FVM_Value missing;
    fvm_value_undefined(&missing);
    stack.push_back(missing);
    return FVM_OK;
}
#endif
