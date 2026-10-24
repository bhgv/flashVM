/* Construct extended Inferno up with active registers in the same owned object. */
#ifndef DISVM_NATIVE_PROCESS_H
#define DISVM_NATIVE_PROCESS_H
#include "native_context.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Bound through native_context_bind; getup() then returns the Proc prefix.
 * [GC:] This initializes native storage, not the full heap, module table or Sys.
 * The retained Proc environment/FPU/error fields are zeroed and need host setup. */
DVM_NativeContext *dvm_native_process_create(void *runtime);
#ifdef __cplusplus
}
#endif
#endif
