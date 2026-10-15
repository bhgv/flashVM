/* FlashVM: invocation-local ConstantPool selection and rooted ownership of referenced strings. */
#include "internal.hpp"

/* Pool selection follows executed branches; lexical proximity never substitutes for runtime state. */
FVM_Result fvm_jit_set_pool(FVM_JitInvocation *s, unsigned int index)
{
    if (index >= s->code->pools.size())
        return FVM_INVALID_ARGUMENT;
    s->pool = &s->code->pools[index].entries;
    return FVM_OK;
}

/* [GC:] Adopt borrowed snapshot/code bytes before publishing the value in the rooted vector. */
FVM_Result fvm_jit_push_pool(FVM_JitInvocation *s, unsigned int index)
{
    if (s->stack->size() >= FVM_JIT_MAX_STACK)
        return FVM_UNSUPPORTED;
    FVM_Value value;
    fvm_value_undefined(&value);
    if (!s->pool || index >= s->pool->size())
    {
        if (!s->context->swf_loaded)
            return FVM_INVALID_ARGUMENT;
    }
    else
    {
        /* Match interpreter own_string's terminated-text policy for inherited pools too. */
        FVM_Result status = fvm_value_string(&value, (*s->pool)[index].c_str());
        if (status == FVM_OK)
            status = fvm_native_result(s->context, FVM_OK, &value);
        if (status != FVM_OK)
            return status;
    }
    s->stack->push_back(value);
    return FVM_OK;
}
