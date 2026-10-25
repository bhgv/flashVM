/* FlashVM: native access to prepared Function2/private and persistent loaded global registers. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"


/* [GC:] The prepared activation or context roots the bank until the copied stack value is live. */
FVM_Result fvm_jit_load_register(FVM_JitInvocation *s, unsigned int index)
{
    if (s->stack->size() >= FVM_JIT_MAX_STACK)
    {
        return FVM_UNSUPPORTED;
    }
    FVM_Value value;
    fvm_value_undefined(&value);
    if (s->frame->registers == s->context->avm1_global_registers)
    {
        FVM_Result status = fvm_exec_global_register(s->context, 0, index, 0, &value);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    else if (index < s->frame->register_count)
    {
        if (!s->frame->registers)
        {
            return FVM_INVALID_ARGUMENT;
        }
        value = s->frame->registers[index];
    }
    else if (!s->context->swf_loaded)
    {
        return FVM_INVALID_ARGUMENT;
    }
    s->stack->push_back(value);
    return FVM_OK;
}


/* [GC:] Keep the operand rooted through global string adoption and publish only on success. */
FVM_Result fvm_jit_store_register(FVM_JitInvocation *s, unsigned int index)
{
    FVM_Result status = fvm_avm1_require_value(s->context->swf_loaded, *s->stack);
    if (status != FVM_OK)
    {
        return status;
    }
    if (s->frame->registers == s->context->avm1_global_registers)
    {
        FVM_Value ignored;
        return fvm_exec_global_register(s->context, 1, index, &s->stack->back(), &ignored);
    }
    if (index < s->frame->register_count)
    {
        if (!s->frame->registers)
        {
            return FVM_INVALID_ARGUMENT;
        }
        s->frame->registers[index] = s->stack->back();
    }
    else if (!s->context->swf_loaded)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return FVM_OK;
}
