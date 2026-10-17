/* FlashVM: borrowed activation metadata; no per-instruction local-value capture. */
#ifndef FVM_CRASH_INTERNAL_HPP
#define FVM_CRASH_INTERNAL_HPP /* Internal diagnostic frame guard. */
#include "runtime.hpp"
/* [GC:] Views borrow values already retained by the ordinary invocation roots.
 * Diagnostic callbacks cannot collect; persistent reports contain only copied bytes. */
struct FVM_CrashActivation
{
    FVM_CrashActivation *previous;  /* Suspended caller link. */
    const fvm_detail::string *name; /* Borrowed call label. */
    const unsigned char *source;    /* Borrowed immutable AVM1 source. */
    size_t source_size;             /* Source body size. */
    unsigned int argc;          /* Supplied arguments, independent of parameter binding mutation. */
    const FVM_Value *argv;      /* [GC:] Existing rooted invocation arguments. */
    FVM_Object *activation;     /* [GC:] Existing rooted local activation, if available. */
    const FVM_Value *registers; /* [GC:] Borrowed rooted register bank. */
    unsigned int register_count;        /* Existing register-bank size. */
    const void *machine;                /* Borrowed RX mapping while active. */
    size_t machine_size;                /* Native payload size. */
    const FVM_ScriptFunction *function; /* Borrowed immutable formal/register metadata. */
    bool forced;                        /* Entry copies were retained successfully in FORCE mode. */
    bool external;        /* Host-owned native attachment, distinct from generated code. */
    bool jit;             /* Actual body engine. */
    unsigned long offset; /* Caller offset frozen on nested entry. */
    unsigned long line;   /* Caller mapped line frozen on nested entry. */
};
/* Link borrowed activation metadata; explicit FORCE mode also owns rooted entry copies. */
class FVM_CrashScope
{
    FVM_Context *context_;                    /* Owning execution context. */
    bool linked_;                             /* Disabled/block scopes do not touch the chain. */
    fvm_detail::vector<FVM_Value> arguments_; /* [GC:] Explicit FORCE copies of supplied values. */
    fvm_detail::vector<fvm_detail::string>
        strings_;                    /* Owned FORCE argument text, never GC-owned. */
    FVM_GCRootFrame argument_roots_; /* [GC:] Retain forced copies across collecting callbacks. */
    bool rooted_;                    /* Whether argument_roots_ is linked. */
    FVM_CrashActivation frame_;      /* Borrowed metadata record valid for this stack scope. */
    FVM_CrashScope(const FVM_CrashScope &);            /* Stack ownership cannot be copied. */
    FVM_CrashScope &operator=(const FVM_CrashScope &); /* Preserve stack link lifetime. */
  public:
    /* Publish metadata only when diagnostics are enabled; reset unwind state at new entries. */
    FVM_CrashScope(FVM_Context *, const fvm_detail::string *, const void *, size_t,
                   unsigned int = 0, const FVM_Value * = 0, FVM_Object * = 0, bool = true);
    /* Restore the suspended link on every return/exception path. */
    ~FVM_CrashScope();
};
/* Capture first-chance VM failures without replacing execution status or throwing. */
void fvm_crash_failure(FVM_Context *, FVM_Result);
#endif
