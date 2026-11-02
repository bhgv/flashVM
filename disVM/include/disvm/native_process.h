/* Construct extended Inferno up with active registers in the same owned object. */
#ifndef DISVM_NATIVE_PROCESS_H
#define DISVM_NATIVE_PROCESS_H
#include "native_context.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* Bound through native_context_bind; getup() then returns the Proc prefix.
 * [GC:] This initializes native storage, not the full heap, module table or Sys.
 * The retained Proc environment/FPU/error fields are zeroed and need host setup. */
DVM_NativeContext *dvm_native_process_create(void *runtime);


/* Host execution guards travel with extended up, never with a saved Prog.REG.
 * Scalars only: neither field is a GC root or a file-format ABI member. */
typedef struct DVM_NativeExecution
{
    unsigned long idle_slices;      /* Owner-local turns since acknowledged input progress. */
    int instruction_boundary_error; /* Quota rejected before PC fetch/advance on this owner. */
    unsigned long progress_epoch;   /* Last shared stdin/UI progress generation observed by this owner. */
    unsigned long host_fcr;         /* Native caller rounding state saved for a managed turn. */
    unsigned long host_fsr;         /* Native caller exception-status bits saved for a managed turn. */
} DVM_NativeExecution;


/* Borrow only this bound extended-up owner's execution guards. Invalid/unbound
 * or register-only contexts are fatal programming errors, just like DVM_R.
 * No allocation, blocking, foreign-context access or managed heap access. */
DVM_NativeExecution *dvm_native_execution(void);
#ifdef __cplusplus
}
#endif
#endif
