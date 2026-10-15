/* FlashVM: context-local execution policy, bounded automatic compilation and synchronous
 * diagnostics. */
#include "internal.hpp"
#include "mode_internal.hpp"
#include "../avm1/block.hpp"
#include "../avm1/handlers/native_body.hpp"
#include "../avm1/handlers/script_call.hpp"
#include "../avm1/target_scope.hpp"
#include "../avm1/encoding_constants.h"
#include "flashvm/exception.h"
#include <cstdio>
/* Release all positive cache entries; no managed object graph edges are retained here. */
FVM_ExecutionModeState::~FVM_ExecutionModeState()
{
    for (fvm_detail::map<fvm_detail::string, FVM_JitCode *>::iterator i = cache.begin();
         i != cache.end(); ++i)
        if (i->second)
            fvm_jit_destroy(i->second);
}
/* Explicitly initialized entry options choose interpretation unless the host selects JIT. */
extern "C" FVM_Result fvm_execution_options_init(FVM_ExecutionOptions *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    out->struct_size = sizeof(*out);
    out->mode = FVM_EXECUTION_INTERPRETER;
    out->flags = 0;
    return FVM_OK;
}
/* Update subsequent entries without invalidating code that an active native frame still uses. */
extern "C" FVM_Result fvm_execution_set_mode(FVM_Context *c, FVM_ExecutionMode mode,
                                             unsigned int flags)
{
    if (!c || c->destroying || c->gc_releasing || mode < FVM_EXECUTION_MANUAL ||
        mode > FVM_EXECUTION_JIT || (flags & ~FVM_EXEC_REPLAY_ON_EXCEPTION))
        return FVM_INVALID_ARGUMENT;
    c->execution_mode.mode = mode;
    c->execution_mode.flags = flags;
    return FVM_OK;
}
/* Read requested policy even while a diagnostic retry is forcing interpretation. */
extern "C" FVM_Result fvm_execution_get_options(FVM_Context *c, FVM_ExecutionOptions *out)
{
    if (!c || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    out->mode = c->execution_mode.mode;
    out->flags = c->execution_mode.flags;
    return FVM_OK;
}
/* Logging state is host-owned and deliberately excluded from graph serialization. */
extern "C" FVM_Result fvm_execution_set_logger(FVM_Context *c, FVM_ExecutionLogger logger,
                                               void *data)
{
    if (!c || c->destroying || (!logger && data))
        return FVM_INVALID_ARGUMENT;
    c->execution_mode.logger = logger;
    c->execution_mode.logger_data = data;
    return FVM_OK;
}
/* Snapshot counters without allocating or entering script. */
extern "C" FVM_Result fvm_execution_get_stats(FVM_Context *c, FVM_ExecutionStats *out)
{
    if (!c || !out || out->struct_size != sizeof(*out))
        return FVM_INVALID_ARGUMENT;
    *out = c->execution_mode.stats;
    return FVM_OK;
}
namespace
{
/* Validate options before changing any policy or executing source. */
FVM_Result options(FVM_Context *c, const FVM_ExecutionOptions *value)
{
    if (!value)
        return c ? FVM_OK : FVM_INVALID_ARGUMENT;
    if (value->struct_size != sizeof(*value))
        return FVM_INVALID_ARGUMENT;
    return fvm_execution_set_mode(c, value->mode, value->flags);
}
/* Diagnostic callbacks borrow stable strings and may not reenter script execution. */
void event(FVM_Context *c, FVM_ExecutionEventKind kind, FVM_Result status, const char *name,
           size_t length, unsigned long offset)
{
    FVM_ExecutionEvent value;
    value.kind = kind;
    value.offset = offset;
    value.function = name;
    value.function_size = length;
    value.status = status;
    if (c->execution_mode.logger)
    {
        try
        {
            c->execution_mode.logger(c, &value, c->execution_mode.logger_data);
        }
        catch (...)
        {
        } /* A host logger cannot unwind across execution cleanup. */
    }
    else
    {
        size_t bounded = length < FVM_JIT_LOG_LABEL_BYTES ? length : FVM_JIT_LOG_LABEL_BYTES;
        std::fprintf(stderr, "FlashVM diagnostic event=%u function=%.*s offset=%lu status=%d\n",
                     (unsigned int)kind, (int)bounded, name, offset, (int)status);
    }
}
/* Synchronous replay task invoked only after the original native frame has returned. */
typedef FVM_Result (*ReplayTask)(void *);
/* Preserve original thrown value/fault and budget through a separate bounded interpreter retry. */
void replay(FVM_Context *c, const char *name, size_t length, unsigned long offset, ReplayTask task,
            void *data)
{
    if (c->execution_mode.replay_depth || !(c->execution_mode.flags & FVM_EXEC_REPLAY_ON_EXCEPTION))
        return;
    try
    {
        /* [GC:] Clearing pending state must not release the original thrown graph. */
        FVM_ExecutionRoots roots(c, FVM_JIT_REPLAY_ROOT_COUNT);
        roots.values[FVM_JIT_REPLAY_EXCEPTION_ROOT] = c->exception_value;
        bool pending = c->exception_pending, fault_pending = c->fault_pending;
        FVM_ExceptionInfo fault = c->fault;
        unsigned long steps = c->avm1_steps;
        ++c->execution_mode.replay_depth;
        ++c->execution_mode.stats.replays;
        event(c, FVM_EXEC_EVENT_NATIVE_EXCEPTION, FVM_EXCEPTION, name, length, offset);
        event(c, FVM_EXEC_EVENT_REPLAY_BEGIN, FVM_OK, name, length, offset);
        fvm_exception_clear(c);
        c->avm1_steps = 0;
        FVM_Result status;
        try
        {
            status = task(data);
        }
        catch (const std::bad_alloc &)
        {
            status = FVM_OUT_OF_MEMORY;
        }
        catch (...)
        {
            status = FVM_ERROR;
        }
        c->execution_mode.stats.last_replay_status = status;
        c->exception_value = roots.values[FVM_JIT_REPLAY_EXCEPTION_ROOT];
        c->exception_pending = pending;
        c->fault_pending = fault_pending;
        c->fault = fault;
        c->avm1_steps = steps;
        event(c, FVM_EXEC_EVENT_REPLAY_END, status, name, length, offset);
        --c->execution_mode.replay_depth;
    }
    catch (...)
    {
    } /* Failure to prepare diagnostics preserves the original pending exception. */
}
/* Original function inputs remain retained by the suspended shared activation builder. */
struct FunctionReplay
{
    FVM_Context *context;               /* Owning suspended context. */
    const fvm_detail::string *name;     /* Borrowed immutable call label. */
    const FVM_ScriptFunction *function; /* Source snapshot from the failed invocation. */
    unsigned int argc;                  /* Original argument count. */
    const FVM_Value *argv; /* [GC:] Original rooted arguments, not mutated local slots. */
    FVM_Object *receiver, *closure, *callee, *base; /* [GC:] Retained invocation environment. */
};
/* Recreate arguments, locals, registers and this through the normal activation builder. */
FVM_Result replay_function(void *raw)
{
    FunctionReplay *r = (FunctionReplay *)raw;
    FVM_Value ignored;
    return fvm_avm1_call_script(r->context, *r->name, *r->function, r->argc, r->argv, r->receiver,
                                &ignored, r->closure, r->callee, r->base);
}
/* Root retry uses original copied source without introducing a function-local scope. */
struct RootReplay
{
    FVM_Context *context; /* Owning context. */
    FVM_JitCode *code;    /* Context-owned source snapshot, stable through this retry. */
    FVM_Object *receiver; /* [GC:] Root receiver retained by the caller's scope. */
    bool bounded;        /* Stored frame records permit implicit End at their boundary. */
};
/* Existing root framing validation and interpreter setup remain authoritative. */
FVM_Result replay_root(void *raw)
{
    RootReplay *r = (RootReplay *)raw;
    if (r->bounded)
        return fvm_avm1_execute_block_receiver(r->context, r->code->source.begin(),
                                                r->code->source.size(), r->receiver);
    return fvm_avm1_execute_receiver(r->context, r->code->source.begin(), r->code->source.size(),
                                     r->receiver);
}
} // namespace
/* Options persist; a running callback can deliberately switch future entries again. */
extern "C" FVM_Result fvm_avm1_execute_options(FVM_Context *c, const void *data, size_t size,
                                               const FVM_ExecutionOptions *value)
{
    FVM_Result status = options(c, value);
    return status == FVM_OK ? fvm_avm1_execute(c, data, size) : status;
}
/* Preserve the ordinary invocation ABI and all shared activation/GC behavior. */
extern "C" FVM_Result fvm_function_invoke_options(FVM_Context *c, FVM_Object *function,
                                                  FVM_Object *receiver, unsigned int argc,
                                                  const FVM_Value *argv, FVM_Value *out,
                                                  const FVM_ExecutionOptions *value)
{
    FVM_Result status = options(c, value);
    return status == FVM_OK ? fvm_function_invoke(c, function, receiver, argc, argv, out) : status;
}
/* Nested diagnostic calls stay interpreted even if a logger or script changes requested mode. */
FVM_ExecutionMode fvm_execution_effective_mode(FVM_Context *c)
{
    return c->execution_mode.replay_depth ? FVM_EXECUTION_INTERPRETER : c->execution_mode.mode;
}
/* Cache positive and unsupported bodies; cache exhaustion falls back without evicting active code.
 */
FVM_Result fvm_execution_select_code(FVM_Context *c, const void *source, size_t size,
                                     FVM_JitCode **out)
{
    *out = 0;
    try
    {
        if (!size || size > FVM_JIT_MAX_SOURCE_BYTES || !fvm_jit_available())
        {
            ++c->execution_mode.stats.fallbacks;
            return FVM_OK;
        }
        fvm_detail::string key((const char *)source, size);
        fvm_detail::map<fvm_detail::string, FVM_JitCode *>::iterator found =
            c->execution_mode.cache.find(key);
        if (found != c->execution_mode.cache.end())
        {
            ++c->execution_mode.stats.cache_hits;
            *out = found->second;
            if (!*out)
                ++c->execution_mode.stats.fallbacks;
            return FVM_OK;
        }
        if (c->execution_mode.cache.size() >= FVM_JIT_CACHE_ENTRIES)
        {
            ++c->execution_mode.stats.fallbacks;
            return FVM_OK;
        }
        FVM_JitCode *code = 0;
        FVM_Result status = fvm_jit_compile(source, size, &code);
        if (status == FVM_OUT_OF_MEMORY)
            return status;
        if (status != FVM_OK)
            code = 0;
        size_t retained = fvm_jit_retained_bytes(code);
        if (code && retained > FVM_JIT_CACHE_BYTES - c->execution_mode.cache_bytes)
        {
            fvm_jit_destroy(code);
            ++c->execution_mode.stats.fallbacks;
            return FVM_OK;
        }
        try
        {
            c->execution_mode.cache[key] = code;
        }
        catch (...)
        {
            if (code)
                fvm_jit_destroy(code);
            throw;
        }
        if (code)
        {
            c->execution_mode.cache_bytes += retained;
            ++c->execution_mode.stats.compiled_bodies;
        }
        else
            ++c->execution_mode.stats.fallbacks;
        *out = code;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
/* Root actions can enter native code without inventing function-local bindings or arguments. */
FVM_Result fvm_execution_try_root(FVM_Context *c, const void *source, size_t size,
                                  FVM_Object *receiver, bool *handled, bool bounded)
{
    *handled = false;
    try
    {
        if (fvm_execution_effective_mode(c) != FVM_EXECUTION_JIT ||
            fvm_debug_has_breakpoints(c, fvm_detail::string()))
            return FVM_OK;
        FVM_JitCode *code = 0;
        FVM_Result status = fvm_execution_select_code(c, source, size, &code);
        if (status != FVM_OK)
        {
            *handled = true;
            return status;
        }
        if (!code)
            return FVM_OK;
        *handled = true;
        /* [GC:] Reset root target selection exactly as a fresh interpreted root block does. */
        FVM_AVM1TargetScope target(c, receiver, false);
        /* [GC:] Unloaded root blocks own a fresh legacy bank; loaded roots use
         * the persistent context bank, matching the interpreter's root entry. */
        FVM_ExecutionRoots registers(c, c->swf_loaded ? 0 : FVM_AVM1_LEGACY_REGISTER_COUNT);
        FVM_ScriptFrame frame;
        std::memset(&frame, 0, sizeof(frame));
        frame.struct_size = sizeof(frame);
        frame.abi_version = FVM_SCRIPT_FRAME_ABI;
        frame.receiver = frame.implicit_this = receiver;
        frame.register_count = c->swf_loaded ? FVM_EXEC_GLOBAL_REGISTER_COUNT
                                             : FVM_AVM1_LEGACY_REGISTER_COUNT;
        frame.registers = c->swf_loaded ? c->avm1_global_registers : registers.values.begin();
        fvm_value_undefined(&frame.super_value);
        FVM_ScriptBodyEntry entry;
        entry.callback = fvm_jit_body;
        entry.userdata = code;
        FVM_Value ignored;
        status = fvm_exec_native_body(c, entry, frame, &ignored);
        if (status == FVM_EXCEPTION)
        {
            RootReplay task;
            task.context = c;
            task.code = code;
            task.receiver = receiver;
            task.bounded = bounded;
            replay(c, "", 0, c->debug_offset, replay_root, &task);
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        *handled = true;
        return FVM_OUT_OF_MEMORY;
    }
}
/* Emit per-checkpoint logs only while a diagnostic interpreter run is active. */
void fvm_execution_trace_step(FVM_Context *c, unsigned long offset)
{
    if (!c->execution_mode.replay_depth)
        return;
    ++c->execution_mode.stats.replay_steps;
    event(c, FVM_EXEC_EVENT_INTERPRETER_STEP, FVM_OK, c->debug_function.c_str(),
          c->debug_function.size(), offset);
}
/* Preserve the native exception even when the diagnostic interpreter succeeds or throws again. */
void fvm_execution_replay_function(FVM_Context *c, const fvm_detail::string &name,
                                   const FVM_ScriptFunction &function, unsigned int argc,
                                   const FVM_Value *argv, FVM_Object *receiver, FVM_Object *closure,
                                   FVM_Object *callee, FVM_Object *base, unsigned long offset)
{
    FunctionReplay task;
    task.context = c;
    task.name = &name;
    task.function = &function;
    task.argc = argc;
    task.argv = argv;
    task.receiver = receiver;
    task.closure = closure;
    task.callee = callee;
    task.base = base;
    replay(c, name.c_str(), name.size(), offset, replay_function, &task);
}
