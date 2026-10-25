/* FlashVM: url semantics for C callers and generated code. */
#include "../encoding_constants.h"
#include "support.hpp"
#include "execution_roots.h"


/* Execute value semantics with validation and allocation failure containment. */
extern "C" FVM_Result fvm_exec_url(FVM_Context *c, const FVM_Value *url, const FVM_Value *target, unsigned int flags)
{
    if (!c || fvm_validate_value(c, url) != FVM_OK || fvm_validate_value(c, target) != FVM_OK ||
        (flags & ~FVM_AVM1_URL_VALID_FLAGS) || (flags & FVM_AVM1_URL_METHOD_MASK) == FVM_AVM1_URL_INVALID_METHOD)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        FVM_ExecutionRoots roots(c, FVM_URL_ROOT_COUNT);
        roots.values[FVM_URL_URL_INPUT_ROOT] = *url;
        roots.values[FVM_URL_TARGET_INPUT_ROOT] = *target;
        FVM_Result status =
            fvm_avm1_string(c, roots.values[FVM_URL_TARGET_INPUT_ROOT], &roots.values[FVM_URL_TARGET_TEXT_ROOT]);
        if (status == FVM_OK)
        {
            status = fvm_avm1_string(c, roots.values[FVM_URL_URL_INPUT_ROOT], &roots.values[FVM_URL_URL_TEXT_ROOT]);
        }
        if (status != FVM_OK)
        {
            return status;
        }
        fvm_value_number(&roots.values[FVM_URL_FLAGS_ROOT], flags);
        /* Copy the sink before delivery so replacement applies to the next request. */
        FVM_NativeEntry sink = c->avm1_url;
        if (!sink.callback)
        {
            return FVM_UNSUPPORTED;
        }
        FVM_Value ignored;
        fvm_value_undefined(&ignored);
        {


            /* [GC:] Host URL callbacks share the ordinary native collection barrier. */
            FVM_NativeCallGuard guard(c);
            status =
                sink.callback(c, c->global, FVM_URL_SINK_ARGUMENT_COUNT, roots.values.begin(), &ignored, sink.userdata);
        }
        if (status != FVM_OK)
        {
            return status;
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
