/* FlashVM: attach context-local generated code without replacing script metadata. */
#include "support.hpp"
/* Validate the complete replacement before publishing any code or capture pointer. */
extern "C" FVM_Result fvm_function_set_body(FVM_Context *c, FVM_Object *function,
                                            FVM_ScriptBody body, void *userdata,
                                            FVM_Object *capture)
{
    if (!c || c->destroying || !function || function->owner != c || function->resource_kind != 32 ||
        !function->invocable.callback || (capture && capture->owner != c) ||
        (!body && (userdata || capture)))
        return FVM_INVALID_ARGUMENT;
    function->script_body.callback = body;
    function->script_body.userdata = userdata;
    /* [GC:] The function becomes the persistent owner of the capture root. */
    function->script_body.capture = capture;
    return FVM_OK;
}
