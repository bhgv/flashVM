/* FlashVM: retained context-global value slots shared by interpreted and generated code. */
#include "support.hpp"
#include "flashvm/standard.h"
/* [GC:] Own retained strings and publish a new root only after preparation succeeds. */
extern "C" FVM_Result fvm_exec_global_register(FVM_Context *c, int write, unsigned int index,
                                               const FVM_Value *input, FVM_Value *out)
{
    if (!c || !out || c->destroying || c->gc_releasing ||
        (write && fvm_validate_value(c, input) != FVM_OK))
        return FVM_INVALID_ARGUMENT;
    if (index >= FVM_EXEC_GLOBAL_REGISTER_COUNT)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    if (!write)
    {
        *out = c->avm1_global_registers[index];
        return FVM_OK;
    }
    if (input->type != FVM_VALUE_STRING)
    {
        /* Scalar/object copies invoke no callbacks or allocation between publication and output. */
        FVM_Value value = *input;
        c->avm1_global_registers[index] = value;
        *out = value;
        return FVM_OK;
    }
    try
    {
        FVM_ExecutionRoots roots(c, 1);
        roots.values[0] = *input;
        if (input->type == FVM_VALUE_STRING)
        {
            FVM_Result status = fvm_value_to_string(c, input, &roots.values[0]);
            if (status != FVM_OK)
                return status;
            roots.values[0].metatable = input->metatable;
        }
        c->avm1_global_registers[index] = roots.values[0];
        *out = roots.values[0];
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
