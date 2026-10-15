/* FlashVM: native target selection preserves rooted operands and activation target restoration. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"

/* [GC:] Keep a dynamic target path rooted through versioned coercion and scene lookup. */
FVM_Result fvm_jit_target_value(FVM_JitInvocation *s, unsigned int)
{
    FVM_Result status = fvm_avm1_require_value(s->context->swf_loaded, *s->stack);
    if (status == FVM_OK)
        status = fvm_exec_select_target(s->context, &s->stack->back());
    if (status == FVM_OK)
        s->stack->pop_back();
    return status;
}
/* [GC:] Code-owned literal bytes stay alive through the shared service's rooted conversion. */
FVM_Result fvm_jit_target_literal(FVM_JitInvocation *s, unsigned int index)
{
    if (index >= s->code->constants.size())
        return FVM_INVALID_ARGUMENT;
    const fvm_detail::string &text = s->code->constants[index].text;
    FVM_Value path;
    FVM_Result status = fvm_value_string_bytes(&path, text.c_str(), text.size());
    return status == FVM_OK ? fvm_exec_select_target(s->context, &path) : status;
}
