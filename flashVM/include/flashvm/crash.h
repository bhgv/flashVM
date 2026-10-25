/* FlashVM: bounded postmortem reports and synchronous live fault inspection. */
#ifndef FLASHVM_CRASH_H
#define FLASHVM_CRASH_H /* Public crash inspection guard. */
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Policies apply to first-chance VM execution failures, before frame unwinding. */
typedef enum FVM_CrashMode
{
    FVM_CRASH_NONE = 0,  /* Preserve ordinary error behavior without report construction. */
    FVM_CRASH_BACKTRACE, /* Retain names, engine, source and typed argument descriptions. */
    FVM_CRASH_ZIP,       /* Also snapshot raw heap/global/class state and available locals. */
    FVM_CRASH_ZIP_CODE,  /* Include emitted bytes of active JIT bodies as well. */
    FVM_CRASH_DEBUGGER   /* Invoke the host debugger while activation storage is still live. */
} FVM_CrashMode;


/* Detail preservation is independent of the failure action selected above. */
typedef enum FVM_CrashDetail
{
    FVM_CRASH_DETAIL_NATURAL = 0, /* Inspect existing values without forcing entry copies. */
    FVM_CRASH_DETAIL_FORCE        /* Retain supplied arguments and expose named register parameters. */
} FVM_CrashDetail;


/* Actual execution engine; native host attachments need not have emitted machine views. */
typedef enum FVM_CrashEngine
{
    FVM_CRASH_ENGINE_INTERPRETER = 0,
    FVM_CRASH_ENGINE_JIT,
    FVM_CRASH_ENGINE_NATIVE
} FVM_CrashEngine;


/* A copied report owns these byte views until the next report, clear or destruction. */
typedef struct FVM_CrashFrame
{
    const char *function;        /* Counted UTF-8/WTF-8 function name; root is <root>. */
    size_t function_size;        /* Function name byte length. */
    const char *source;          /* Counted host source mapping. */
    size_t source_size;          /* Source name byte length. */
    unsigned long offset;        /* AVM1 action offset, not a native program counter. */
    unsigned long line;          /* Host-mapped line or zero. */
    int jit;                     /* Nonzero for emitted native execution, zero for other engines. */
    int forced;                  /* Nonzero when extra entry preservation succeeded for this frame. */
    unsigned int argc;           /* Original supplied argument count. */
    const void *bytecode;        /* Owned copy of the interpreted source for this activation. */
    size_t bytecode_size;        /* Original action-body byte count. */
    const void *machine_address; /* Original RX load address, an opaque postmortem identity. */
    const void *machine_code;    /* Owned native bytes only in ZIP_CODE mode. */
    size_t machine_code_size;    /* Dumped mapping payload, excluding page padding. */
} FVM_CrashFrame;


/* Runs on the VM thread before unwinding; returning continues normal error propagation.
     * May inspect through crash/table/debug APIs; must not execute script, collect, mutate or
     * destroy the context. Block in the callback to keep the VM paused for a host debugger. */
typedef void (*FVM_CrashHandler)(FVM_Context *, FVM_Result, void *);


/* Copy policy/path. A null path retains ZIP bytes in memory without writing a file.
     * File path is overwritten on each report. Handler is used only in DEBUGGER mode. */
FVM_API FVM_Result fvm_crash_configure(FVM_Context *, FVM_CrashMode, const char *path,
                                       FVM_CrashHandler, void *);


/* Set detail policy outside execution. FORCE adds entry allocations/roots, but keeps native
     * JIT execution. Current backends materialize all locals/registers already; no per-step spills
     * are added in NATURAL mode. Failed force preservation is exposed through diagnostic_status. */
FVM_API FVM_Result fvm_crash_set_detail(FVM_Context *, FVM_CrashDetail);


/* Clear the retained report outside a debugger callback. */
FVM_API FVM_Result fvm_crash_clear(FVM_Context *);


/* Query retained report metadata; status describes the original VM failure.
     * diagnostic_status reports capture/file errors without replacing that failure. */
FVM_API FVM_Result fvm_crash_info(FVM_Context *, size_t *frames, FVM_Result *status,
                                  FVM_Result *diagnostic_status);


/* Copy a borrowed frame view, innermost first. */
FVM_API FVM_Result fvm_crash_frame(FVM_Context *, size_t, FVM_CrashFrame *);


/* Query exact engine classification, including host-owned native attachments. */
FVM_API FVM_Result fvm_crash_frame_engine(FVM_Context *, size_t, FVM_CrashEngine *);


/* Borrow the copied executed source line when an attached SWD resolves the frame. */
FVM_API FVM_Result fvm_crash_source_line(FVM_Context *, size_t, const char **, size_t *);


/* Retained JSON trace or ZIP bytes. Output is borrowed; zero size denotes no ZIP. */
FVM_API FVM_Result fvm_crash_data(FVM_Context *, int zip, const void **, size_t *);


/* Live breakpoint/fault debugger inspection. Values and names are borrowed until callback
     * return. No getters or coercions are invoked. Registers have no invented local names. */
FVM_API FVM_Result fvm_crash_argument(FVM_Context *, size_t frame, unsigned int, FVM_Value *);


/* Enumerate available raw activation locals; NOT_FOUND marks end/unavailable. */
FVM_API FVM_Result fvm_crash_local(FVM_Context *, size_t frame, size_t index, const char **name,
                                   size_t *name_size, FVM_Value *);


/* Read a live activation register by its ABI slot. */
FVM_API FVM_Result fvm_crash_register(FVM_Context *, size_t frame, unsigned int, FVM_Value *);
#ifdef __cplusplus
}
#endif
#endif
