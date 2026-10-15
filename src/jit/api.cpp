/* FlashVM: explicit native program ownership and prepared-activation entry. */
#include "internal.hpp"
#include <cstring>
/* Availability is limited to the implemented ABI, not merely the processor family. */
extern "C" int fvm_jit_available(void)
{
    return fvm_port_code_supported() && sizeof(FVM_JitStep) == FVM_JIT_ADDRESS_BYTES &&
           sizeof(void *) == FVM_JIT_ADDRESS_BYTES;
}
/* Publish a handle only after complete decoding, relocation and W^X sealing. */
extern "C" FVM_Result fvm_jit_compile(const void *bytes, size_t size, FVM_JitCode **out)
{
    if (!bytes || !size || !out)
        return FVM_INVALID_ARGUMENT;
    if (!fvm_jit_available())
        return FVM_UNSUPPORTED;
    if (size > FVM_JIT_MAX_SOURCE_BYTES)
        return FVM_UNSUPPORTED;
    FVM_JitCode *code = 0;
    try
    {
        code = new FVM_JitCode;
        code->source.resize(size);
        std::memcpy(code->source.begin(), bytes, size);
        FVM_Result status = fvm_jit_decode(code->source.begin(), size, *code);
        if (status == FVM_OK)
            status = fvm_jit_emit(*code);
        if (status != FVM_OK)
        {
            delete code;
            return status;
        }
        *out = code;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        delete code;
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        delete code;
        return FVM_ERROR;
    }
}
/* Enter a real native block only after every transient operand root has been installed. */
extern "C" FVM_Result fvm_jit_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out,
                                   void *userdata)
{
    FVM_JitCode *code = (FVM_JitCode *)userdata;
    if (!c || c->destroying || c->gc_releasing || !frame || !out || !code || !code->memory ||
        frame->struct_size != sizeof(*frame) || frame->abi_version != FVM_SCRIPT_FRAME_ABI ||
        !c->avm1_execution_depth)
        return FVM_INVALID_ARGUMENT;
    /* Same-thread reentry may share immutable code, but may not release active instruction pages.
     */
    struct Active
    {
        FVM_JitCode *code; /* Active mapping protected until native return. */
        /* Reject release until the outermost native invocation returns. */
        explicit Active(FVM_JitCode *p) : code(p)
        {
            ++code->active;
        }
        /* Restore active mapping ownership on success and every failure. */
        ~Active()
        {
            --code->active;
        }
    } active(code);
    try
    {
        /* [GC:] The root vector object stays stable while its operand storage grows. */
        FVM_ExecutionRoots roots(c, 0);
        FVM_JitInvocation state;
        state.context = c;
        state.frame = frame;
        state.code = code;
        state.stack = &roots.values;
        state.steps = 0;
        state.condition = false;
        fvm_value_undefined(&state.result);
        typedef FVM_Result (*Entry)(FVM_JitInvocation *);
        Entry entry;
        std::memcpy(&entry, &code->memory, sizeof(entry));
        ++c->execution_mode.stats.native_entries;
        FVM_Result status = entry(&state);
        if (status == FVM_OK)
            *out = state.result;
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
/* Detachment is the host's responsibility, matching the native-body userdata ownership ABI. */
extern "C" FVM_Result fvm_jit_destroy(FVM_JitCode *code)
{
    if (!code || code->active)
        return FVM_INVALID_ARGUMENT;
    delete code;
    return FVM_OK;
}
/* Report actual emitted bytes rather than page-rounded allocation size. */
extern "C" size_t fvm_jit_code_size(const FVM_JitCode *code)
{
    return code ? code->bytes : 0;
}
