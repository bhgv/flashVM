#ifndef FLASHVM_FUNCTION_H
#define FLASHVM_FUNCTION_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* First-class, context-owned native or AVM1 functions. Native code requires a
     * stable callback codec before it can be transferred to another context. */
    FVM_API FVM_Result fvm_function_create_native(FVM_Context *, FVM_NativeFunction, void *,
                                                  FVM_Object *capture, FVM_Object **);
    FVM_API FVM_Result fvm_function_create_avm1(FVM_Context *, const void *actions, size_t size,
                                                unsigned int parameter_count,
                                                const char *const *parameters,
                                                FVM_Object *environment, FVM_Object **);
    FVM_API FVM_Result fvm_function_invoke(FVM_Context *, FVM_Object *function,
                                           FVM_Object *receiver, unsigned int argc,
                                           const FVM_Value *, FVM_Value *);
    FVM_API FVM_Result fvm_function_construct(FVM_Context *, FVM_Object *, unsigned int,
                                              const FVM_Value *, FVM_Object **);
#ifdef __cplusplus
}
#endif
#endif
