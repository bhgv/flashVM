/* FlashVM: trace semantics for C callers and generated code. */
#include "support.hpp"
/* Execute value semantics with validation and allocation failure containment. */
extern "C" FVM_Result fvm_exec_trace(FVM_Context *c, const FVM_Value *input)
{
    if (!c || fvm_validate_value(c, input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, 1);
        roots.values[0] = *input;
        /* [GC:] The input remains in the temporary roots during toString execution. */
        FVM_Value text, ignored;
        FVM_Result status = roots.values[0].type == FVM_VALUE_UNDEFINED
                                ? fvm_value_string(&text, "undefined")
                                : fvm_avm1_string(c, roots.values[0], &text);
        if (status != FVM_OK)
            return status;
        FVM_NativeEntry sink = c->avm1_trace;
        if (sink.callback)
        {
            fvm_value_undefined(&ignored);
            /* [GC:] Prevent collection while the copied sink/capture and text are borrowed. */
            FVM_NativeCallGuard guard(c);
            status = sink.callback(c, c->global, 1, &text, &ignored, sink.userdata);
            if (status != FVM_OK)
                return status;
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
