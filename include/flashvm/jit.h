/* FlashVM: explicit experimental native compilation with interpreter fallback under host control.
 */
#ifndef FLASHVM_JIT_H
#define FLASHVM_JIT_H /* Public native compiler interface guard. */
#include "function.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Immutable host-owned code; does not retain a context or managed objects. */
    typedef struct FVM_JitCode FVM_JitCode;
    /* Report whether this build/host can emit and execute the current native backend. */
    FVM_API int fvm_jit_available(void);
    /* Static Linux ABI name for diagnostics, or "unsupported"; no allocation or ownership. */
    FVM_API const char *fvm_jit_backend_name(void);
    /* Compile an action body without changing any function. Unsupported operations return
     * UNSUPPORTED; malformed supported records return INVALID_ARGUMENT. Errors preserve out.
     * On success the host owns code and must keep it alive while attached to any function.
     * Attach fvm_jit_body with code as userdata through fvm_function_set_body. Compile the
     * same body used to create the function; constant pools follow the prepared activation. */
    FVM_API FVM_Result fvm_jit_compile(const void *actions, size_t size, FVM_JitCode **out);
    /* Prepared-function body ABI. Calls native code with rooted temporaries and shared budgets.
     * Use through ordinary function invocation, not as an alternative activation builder. */
    FVM_API FVM_Result fvm_jit_body(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *,
                                    void *code);
    /* Release code after unbinding all functions. Active invocation prevents destruction.
     * The host must not reuse a released handle or leave it attached to an inactive function. */
    FVM_API FVM_Result fvm_jit_destroy(FVM_JitCode *);
    /* Return emitted bytes including owned nested blocks; zero for null. No writable memory. */
    FVM_API size_t fvm_jit_code_size(const FVM_JitCode *);
#ifdef __cplusplus
}
#endif
#endif
