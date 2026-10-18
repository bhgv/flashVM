/* FlashVM: runtime execution policy, diagnostic replay and lightweight counters. */
#ifndef FLASHVM_EXECUTION_MODE_H
#define FLASHVM_EXECUTION_MODE_H /* Public execution policy guard. */
#include "avm1.h"
#include "jit.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Manual preserves existing explicit native-body attachments; otherwise interprets. */
    typedef enum FVM_ExecutionMode
    {
        FVM_EXECUTION_MANUAL = 0,      /* Compatibility mode; initial policy in INTERPRETER builds. */
        FVM_EXECUTION_INTERPRETER = 1, /* Bypass JIT bodies and automatic compilation. */
        FVM_EXECUTION_JIT = 2          /* Compile supported bodies; interpret unsupported ones. */
    } FVM_ExecutionMode;
#define FVM_EXEC_REPLAY_ON_EXCEPTION                                                               \
    1U /* Opt-in diagnostic retry; side effects are not rolled back. */
    /* Entry options persist in the context, including changes made by a running callback. */
    typedef struct FVM_ExecutionOptions
    {
        unsigned long struct_size; /* Must equal sizeof this structure. */
        FVM_ExecutionMode mode;    /* Requested policy for subsequent body entries. */
        unsigned int flags;        /* Zero or REPLAY_ON_EXCEPTION. */
    } FVM_ExecutionOptions;
    /* Monotonic counters; unsigned overflow follows the host unsigned-long representation. */
    typedef struct FVM_ExecutionStats
    {
        unsigned long struct_size;         /* Caller supplies sizeof this structure. */
        unsigned long native_entries;      /* Actual entries into emitted machine code. */
        unsigned long interpreter_entries; /* Bytecode-body/block entries. */
        unsigned long compiled_bodies;     /* Successfully cached automatic compilations. */
        unsigned long cache_hits;   /* Reused automatic positive or negative cache entries. */
        unsigned long fallbacks;    /* Automatic unsupported/cache-limit interpreter selections. */
        unsigned long replays;      /* Diagnostic interpreter retries after a native exception. */
        unsigned long replay_steps; /* Checkpoints executed while diagnostic replay is active. */
        FVM_Result last_replay_status; /* Status produced by the last diagnostic retry. */
    } FVM_ExecutionStats;
    /* Logs are emitted only for diagnostic replay, not on the ordinary native fast path. */
    typedef enum FVM_ExecutionEventKind
    {
        FVM_EXEC_EVENT_NATIVE_EXCEPTION =
            1,                           /* Original native attempt raised a script exception. */
        FVM_EXEC_EVENT_REPLAY_BEGIN,     /* Start a fresh interpreter activation. */
        FVM_EXEC_EVENT_INTERPRETER_STEP, /* Original byte offset inside the diagnostic retry. */
        FVM_EXEC_EVENT_REPLAY_END /* Diagnostic retry completed; original exception is restored. */
    } FVM_ExecutionEventKind;
    /* Event/string storage is borrowed only for the synchronous callback. */
    typedef struct FVM_ExecutionEvent
    {
        FVM_ExecutionEventKind kind; /* Diagnostic event identifier. */
        unsigned long offset;        /* Original bytecode offset. */
        const char *function;        /* Counted function label; empty for root actions. */
        size_t function_size;        /* Label byte count, not including any terminator. */
        FVM_Result status;           /* Failure/retry status, or OK for a checkpoint. */
    } FVM_ExecutionEvent;
    /* Host-owned logger/data. May query counters or change mode; must not reenter execution,
     * collect, throw or destroy the context. Null logger prints diagnostics to stderr. */
    typedef void (*FVM_ExecutionLogger)(FVM_Context *, const FVM_ExecutionEvent *, void *);
    /* Initialize ABI size, interpreter mode and no replay. */
    FVM_API FVM_Result fvm_execution_options_init(FVM_ExecutionOptions *);
    /* Change future entries; an active native frame finishes its selected body. Diagnostic
     * replay forces interpreter entries until it ends. Requested mode changes still persist. */
    FVM_API FVM_Result fvm_execution_set_mode(FVM_Context *, FVM_ExecutionMode, unsigned int flags);
    /* Read requested mode/options without callbacks. New contexts start in JIT for
     * DEFS JIT builds and MANUAL for DEFS INTERPRETER builds. */
    FVM_API FVM_Result fvm_execution_get_options(FVM_Context *, FVM_ExecutionOptions *);
    /* Register synchronous diagnostic logging, or restore the stderr logger with null/null. */
    FVM_API FVM_Result fvm_execution_set_logger(FVM_Context *, FVM_ExecutionLogger, void *);
    /* Snapshot context-local execution and diagnostic counters. */
    FVM_API FVM_Result fvm_execution_get_stats(FVM_Context *, FVM_ExecutionStats *);
    /* Set options before entering root actions; null options retain the current mode. */
    FVM_API FVM_Result fvm_avm1_execute_options(FVM_Context *, const void *, size_t,
                                                const FVM_ExecutionOptions *);
    /* Set options before the normal rooted function activation builder. */
    FVM_API FVM_Result fvm_function_invoke_options(FVM_Context *, FVM_Object *, FVM_Object *,
                                                   unsigned int, const FVM_Value *, FVM_Value *,
                                                   const FVM_ExecutionOptions *);
#ifdef __cplusplus
}
#endif
#endif
