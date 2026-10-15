/* FlashVM: native method calls retain AVM1 operand ordering and shared call semantics. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "../avm1/operand_constants.h"

/* [GC:] Keep owner, name and arguments rooted until shared dispatch returns its adopted result. */
FVM_Result fvm_jit_call_method(FVM_JitInvocation *s, unsigned int construct)
{
    FVM_Context *c = s->context;
    fvm_detail::vector<FVM_Value> &stack = *s->stack;
    FVM_Result status = fvm_avm1_require_operands(c->swf_loaded, stack,
                                                FVM_AVM1_METHOD_FIXED_OPERANDS);
    if (status != FVM_OK)
        return status;
    if (!c->swf_loaded && stack.back().type != FVM_VALUE_STRING &&
        stack.back().type != FVM_VALUE_NULL && stack.back().type != FVM_VALUE_UNDEFINED)
        return FVM_ERROR;

    unsigned int argc = 0;
    status = fvm_exec_argument_count(c, &stack[stack.size() - FVM_AVM1_METHOD_FIXED_OPERANDS],
                                     stack.size() - FVM_AVM1_METHOD_FIXED_OPERANDS, &argc);
    if (status != FVM_OK)
        return status;
    size_t first = stack.size() - argc - FVM_AVM1_METHOD_FIXED_OPERANDS;
    const FVM_Value &owner = stack[stack.size() - FVM_AVM1_METHOD_OWNER_OFFSET];
    FVM_Value result;
    fvm_value_undefined(&result);
    if ((construct && !c->swf_loaded) ||
        (owner.type != FVM_VALUE_NULL && owner.type != FVM_VALUE_UNDEFINED))
    {
        fvm_detail::vector<FVM_Value> args(argc);
        for (unsigned int i = 0; i < argc; ++i)
            args[i] = stack[stack.size() - FVM_AVM1_METHOD_FIRST_ARGUMENT - i];
        status = fvm_exec_call_method(c, construct != 0, &owner, &stack.back(),
                                      s->frame->receiver, argc, argc ? args.begin() : 0, &result);
        if (status != FVM_OK)
            return status;
    }
    /* [GC:] Publish before shrinking; no allocation occurs after the service releases its roots. */
    stack[first] = result;
    stack.resize(first + 1);
    return FVM_OK;
}
