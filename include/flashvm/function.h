/* FlashVM: Public embedding API declarations: function.
 * File: include/flashvm/function.h.
 */

#ifndef FLASHVM_FUNCTION_H
/* Define FLASHVM FUNCTION H constant or compilation switch. */
#define FLASHVM_FUNCTION_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* First-class, context-owned native or AVM1 functions. Native code requires a
     * stable callback codec before it can be transferred to another context. */
    /* Create function native. */
    FVM_API FVM_Result fvm_function_create_native(FVM_Context *, FVM_NativeFunction, void *,
                                                  FVM_Object *capture, FVM_Object **);
    /* Create function avm1. */
    FVM_API FVM_Result fvm_function_create_avm1(FVM_Context *, const void *actions, size_t size,
                                                unsigned int parameter_count,
                                                const char *const *parameters,
                                                FVM_Object *environment, FVM_Object **);
    /* Create AVM1 with string-valued counted parameter names, including embedded NUL.
     * UTF-16-equivalent formals share one lexical binding and rightmost arguments alias. */
    FVM_API FVM_Result fvm_function_create_avm1_bytes(FVM_Context *, const void *, size_t,
                                                      unsigned int, const FVM_Value *,
                                                      FVM_Object *, FVM_Object **);
    /* [GC:] Invoke with active function/capture roots. Successful native strings become
       context-owned snapshots; returned objects and primitive metatables must belong to this
       context. Callers must root retained results across GC, including strings in traced mode. */
    /* Invoke function. */
    FVM_API FVM_Result fvm_function_invoke(FVM_Context *, FVM_Object *function,
                                           FVM_Object *receiver, unsigned int argc,
                                           const FVM_Value *, FVM_Value *);
    /* Implement function construct for this subsystem. */
    FVM_API FVM_Result fvm_function_construct(FVM_Context *, FVM_Object *, unsigned int,
                                              const FVM_Value *, FVM_Object **);
#ifdef __cplusplus
}
#endif
#endif
