/* FlashVM: native class operations share rooted prototype, interface and membership services. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "../avm1/operand_constants.h"


/* [GC:] Retain both constructors through boxing and prototype accessors. */
FVM_Result fvm_jit_extends(FVM_JitInvocation *s, unsigned int)
{
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack, FVM_AVM1_EXTENDS_OPERANDS);
    if (status != FVM_OK)
    {
        return status;
    }
    size_t first = s->stack->size() - FVM_AVM1_EXTENDS_OPERANDS;
    status = fvm_exec_extends(s->context, &(*s->stack)[first], &s->stack->back());
    if (status == FVM_OK)
    {
        s->stack->resize(first);
    }
    return status;
}


/* [GC:] Preserve count, constructor and interfaces until shared dispatch publishes edges. */
FVM_Result fvm_jit_implements(FVM_JitInvocation *s, unsigned int)
{
    FVM_Context *c = s->context;
    fvm_detail::vector<FVM_Value> &stack = *s->stack;
    FVM_Result status = fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_IMPLEMENTS_FIXED_OPERANDS);
    if (status != FVM_OK)
    {
        return status;
    }
    unsigned int count = 0;
    status = fvm_exec_interface_count(c,
                                      &stack[stack.size() - FVM_AVM1_IMPLEMENTS_FIXED_OPERANDS],
                                      stack.size() - FVM_AVM1_IMPLEMENTS_FIXED_OPERANDS,
                                      &count);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_detail::vector<FVM_Value> interfaces(count);
    for (unsigned int i = 0; i < count; ++i)
    {
        interfaces[i] = stack[stack.size() - FVM_AVM1_IMPLEMENTS_FIRST_INTERFACE - i];
    }
    status = fvm_exec_implements(c, &stack.back(), count, count ? interfaces.begin() : 0);
    if (status == FVM_OK)
    {
        stack.resize(stack.size() - count - FVM_AVM1_IMPLEMENTS_FIXED_OPERANDS);
    }
    return status;
}


/* [GC:] Preserve Cast's distinct class/value order through boxing and membership lookup. */
FVM_Result fvm_jit_class_match(FVM_JitInvocation *s, unsigned int cast)
{
    size_t count = cast ? FVM_AVM1_CAST_OPERANDS : FVM_AVM1_INSTANCE_OF_OPERANDS;
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack, count);
    if (status != FVM_OK)
    {
        return status;
    }
    size_t first = s->stack->size() - count;
    FVM_Value result;
    status = cast ? fvm_exec_cast(s->context, &s->stack->back(), &(*s->stack)[first], &result)
                  : fvm_exec_instance_of(s->context, &(*s->stack)[first], &s->stack->back(), &result);
    if (status == FVM_OK)
    {
        (*s->stack)[first] = result;
        s->stack->pop_back();
    }
    return status;
}
