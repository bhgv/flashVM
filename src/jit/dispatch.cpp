/* FlashVM: native URL delivery and frame calls share rooted host and timeline dispatch. */
#include "internal.hpp"
#include "url_constants.h"
#include "../avm1/operand_stack.hpp"
#include "../avm1/operand_constants.h"
#include "../avm1/encoding_constants.h"
#include "../swf/format_constants.h"
#include "flashvm/movieclip.h"
/* [GC:] Adopt both code-owned byte strings before synchronous delivery can retain them. */
FVM_Result fvm_jit_url_literal(FVM_JitInvocation *s, unsigned int index)
{
    FVM_Context *c = s->context;
    if (index >= s->code->constants.size() ||
        s->code->constants.size() - index < FVM_JIT_URL_LITERAL_CONSTANT_COUNT)
        return FVM_INVALID_ARGUMENT;
    FVM_ExecutionRoots roots(c, FVM_AVM1_URL_REQUEST_ROOT_COUNT);
    for (unsigned int i = 0; i < FVM_JIT_URL_LITERAL_CONSTANT_COUNT; ++i)
    {
        const fvm_detail::string &text = s->code->constants[index + i].text;
        if (c->swf_loaded && c->swf_info.version < FVM_SWF_VERSION_6)
            for (size_t at = 0; at < text.size(); ++at)
                if ((unsigned char)text[at] > FVM_AVM1_URL_LEGACY_MAX_BYTE)
                    return FVM_UNSUPPORTED;
    }
    const unsigned int offsets[] = {FVM_JIT_URL_LITERAL_URL_OFFSET, FVM_JIT_URL_LITERAL_TARGET_OFFSET};
    const unsigned int slots[] = {FVM_AVM1_URL_REQUEST_URL_ROOT, FVM_AVM1_URL_REQUEST_TARGET_ROOT};
    for (unsigned int i = 0; i < FVM_JIT_URL_LITERAL_CONSTANT_COUNT; ++i)
    {
        const fvm_detail::string &text = s->code->constants[index + offsets[i]].text;
        FVM_Value &value = roots.values[slots[i]];
        FVM_Result status = fvm_value_string_bytes(&value, text.c_str(), text.size());
        if (status == FVM_OK)
            status = fvm_native_result(c, FVM_OK, &value);
        if (status != FVM_OK)
            return status;
    }
    return fvm_exec_url(c, &roots.values[FVM_AVM1_URL_REQUEST_URL_ROOT],
                         &roots.values[FVM_AVM1_URL_REQUEST_TARGET_ROOT], 0);
}
/* [GC:] Keep URL and target on the rooted vector through target-first conversion and host errors. */
FVM_Result fvm_jit_url_values(FVM_JitInvocation *s, unsigned int flags)
{
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack, FVM_AVM1_URL_OPERANDS);
    if (status != FVM_OK)
        return status;
    size_t first = s->stack->size() - FVM_AVM1_URL_OPERANDS;
    status = fvm_exec_url(s->context, &(*s->stack)[first], &s->stack->back(), flags);
    if (status == FVM_OK)
        s->stack->resize(first);
    return status;
}
/* [GC:] Retain the original frame request until the shared nested frame dispatch finishes. */
FVM_Result fvm_jit_call_frame(FVM_JitInvocation *s, unsigned int)
{
    FVM_Context *c = s->context;
    FVM_Result status = fvm_avm1_require_value(c->swf_loaded, *s->stack);
    if (status != FVM_OK)
        return status;
    FVM_Object *target = s->frame->receiver;
    if (c->avm1_target_active && c->avm1_base_clip)
    {
        target = c->avm1_target_clip;
        if (!target)
        {
            FVM_Value root;
            status = fvm_value_string(&root, "/");
            if (status == FVM_OK)
                status = fvm_movieclip_resolve_path(c, c->avm1_base_clip, root, &target);
        }
    }
    if (status == FVM_OK)
        status = fvm_exec_call_frame(c, target, &s->stack->back());
    if (status == FVM_OK)
        s->stack->pop_back();
    return status;
}
