/* FlashVM: explicit native program ownership and prepared-activation entry. */
#include "internal.hpp"
#include "../core/crash_internal.hpp"
#include <cstring>
#include "../avm1/encoding_constants.h"


/* Availability is limited to the implemented ABI, not merely the processor family. */
extern "C" int fvm_jit_available(void)
{
    return fvm_port_code_supported() && sizeof(FVM_JitStep) == sizeof(void *) &&
           fvm_jit_target_width(fvm_jit_host_target()) == sizeof(void *);
}


/* Publish a handle only after complete decoding, relocation and W^X sealing. */
FVM_Result fvm_jit_compile_block(const void *bytes, size_t size, FVM_JitCode **out, unsigned int depth, bool nested)
{
    if ((!bytes && size) || (!nested && !size) || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!fvm_jit_available())
    {
        return FVM_UNSUPPORTED;
    }
    if (size > FVM_JIT_MAX_SOURCE_BYTES || depth > FVM_AVM1_MAX_BLOCK_DEPTH)
    {
        return FVM_UNSUPPORTED;
    }
    FVM_JitCode *code = 0;
    try
    {
        code = new FVM_JitCode;
        code->source.resize(size);
        if (size)
        {
            std::memcpy(code->source.begin(), bytes, size);
        }
        FVM_Result status = fvm_jit_decode(code->source.begin(), size, *code, depth, nested);
        if (status == FVM_OK)
        {
            status = fvm_jit_emit(*code);
        }
        if (status == FVM_OK && fvm_jit_retained_bytes(code) > FVM_JIT_MAX_CODE_BYTES)
        {
            status = FVM_UNSUPPORTED;
        }
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


/* Ordinary public compilation retains nonempty source validation and root End accounting. */
extern "C" FVM_Result fvm_jit_compile(const void *bytes, size_t size, FVM_JitCode **out)
{
    return fvm_jit_compile_block(bytes, size, out, 0, false);
}


/* All owned block mappings participate in compilation/cache byte limits. */
size_t fvm_jit_retained_bytes(const FVM_JitCode *code)
{
    if (!code)
    {
        return 0;
    }
    size_t total = code->bytes;
    if (total > FVM_JIT_MAX_CODE_BYTES)
    {
        return FVM_JIT_MAX_CODE_BYTES + 1;
    }
    for (size_t i = 0; i < code->blocks.size(); ++i)
    {
        size_t child = fvm_jit_retained_bytes(code->blocks[i].code);
        if (child > FVM_JIT_MAX_CODE_BYTES - total)
        {
            return FVM_JIT_MAX_CODE_BYTES + 1;
        }
        total += child;
    }
    return total;
}


/* Enter a real native block only after every transient operand root has been installed. */
extern "C" FVM_Result fvm_jit_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *userdata)
{
    FVM_JitCode *code = (FVM_JitCode *)userdata;
    if (!c || c->destroying || c->gc_releasing || !frame || !out || !code || !code->memory ||
        frame->struct_size != sizeof(*frame) || frame->abi_version != FVM_SCRIPT_FRAME_ABI || !c->avm1_execution_depth)
    {
        return FVM_INVALID_ARGUMENT;
    }


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


        FVM_CrashScope diagnostic(c,
                                  0,
                                  code->source.begin(),
                                  code->source.size(),
                                  frame->argc,
                                  frame->argv,
                                  frame->activation,
                                  !frame->activation || !c->crash.active);
        if (c->crash.active && !c->crash.busy && !c->execution_mode.replay_depth)
        {
            FVM_CrashActivation *a = c->crash.active;
            a->jit = true;
            a->source = code->source.begin();
            a->source_size = code->source.size();
            a->machine = code->memory;
            a->machine_size = code->bytes;
            a->registers = frame->registers;
            a->register_count = frame->register_count;
        }
        FVM_JitInvocation state;
        state.context = c;
        state.frame = frame;
        state.code = code;
        state.stack = &roots.values;
        state.pool = c->execution_mode.native_pool;
        state.steps = 0;
        state.condition = false;
        state.returned = false;
        state.source_base = 0;
        fvm_value_undefined(&state.result);


        typedef FVM_Result (*Entry)(FVM_JitInvocation *);
        Entry entry;
        std::memcpy(&entry, &code->memory, sizeof(entry));
        ++c->execution_mode.stats.native_entries;
        FVM_Result status = entry(&state);
        fvm_crash_failure(c, status);
        if (status == FVM_OK)
        {
            *out = state.result;
        }
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
    {
        return FVM_INVALID_ARGUMENT;
    }
    delete code;
    return FVM_OK;
}


/* Report all owned emitted bytes rather than page-rounded mapping sizes. */
extern "C" size_t fvm_jit_code_size(const FVM_JitCode *code)
{
    return fvm_jit_retained_bytes(code);
}


/* Return the immutable AVM1 snapshot associated with emitted code. */
extern "C" FVM_Result fvm_jit_source(const FVM_JitCode *code, const void **out, size_t *size)
{
    if (!code || !out || !size)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = code->source.begin();
    *size = code->source.size();
    return FVM_OK;
}


/* Expose a read-only mapping view for host debugger address correlation. */
extern "C" FVM_Result fvm_jit_machine_code(const FVM_JitCode *code, const void **out, size_t *size)
{
    if (!code || !out || !size)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = code->memory;
    *size = code->bytes;
    return FVM_OK;
}
