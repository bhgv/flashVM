/* Construct extended Inferno up with active registers in the same owned object. */
#ifndef DISVM_NATIVE_PROCESS_H
#define DISVM_NATIVE_PROCESS_H
#include "native_context.h"
#include <fenv.h>

#ifdef __cplusplus
extern "C"
{
#endif


/* Bound through native_context_bind; getup() then returns the Proc prefix.
 * [GC:] This initializes native storage, not the full heap, module table or Sys.
 * The retained Proc environment/FPU/error fields are zeroed and need host setup. */
DVM_NativeContext *dvm_native_process_create(void *runtime);


/* Host execution guards travel with extended up, never with a saved Prog.REG.
 * Scalars only: no field is a GC root or a file-format ABI member. */
typedef struct DVM_NativeExecution
{
    unsigned long idle_slices;      /* Owner-local turns since acknowledged input progress. */
    int arithmetic_retry_opcode;    /* Armed decoded opcode, or zero outside dispatch; -1 requests retry. */
    int instruction_boundary_error; /* Quota rejected before PC fetch/advance on this owner. */
    unsigned long progress_epoch;   /* Last shared stdin/UI progress generation observed by this owner. */
    fenv_t host_fp_environment;      /* Native caller FP environment saved for a managed turn. */
} DVM_NativeExecution;


/* Borrow only this bound extended-up owner's execution guards. Invalid/unbound
 * or register-only contexts are fatal programming errors, just like DVM_R.
 * No allocation, blocking, foreign-context access or managed heap access. */
DVM_NativeExecution *dvm_native_execution(void);


/* Capture owner-local native cleanup checkpoint before caller's setjmp. Return
 * 0 unchanged for invalid/overflow/guarded state, 1 after pushing Proc.nerr.
 * Host macros only: no jump is performed inside this function. */
int dvm_native_error_push(void);


/* Pop normal error label only with no newer registered scopes. Return 0 without
 * mutation on pending cleanup/invalid owner stack; caller must not ignore it. */
int dvm_native_error_pop(void);


/* Unwind newer native scopes before caller longjmps. Return 1 after consuming
 * label, 0 on invalid role/protocol. Failure is fatal, possibly after partial
 * callback cleanup; never treat it as permission to resume managed execution. */
int dvm_native_error_unwind(void);


#ifdef __cplusplus
}
#endif
#endif
