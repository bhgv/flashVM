#ifndef FLASHVM_METAMETHOD_H
#define FLASHVM_METAMETHOD_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Native metamethod callbacks receive the context and positional values:
     * [0] owner table, [1] defining metatable, followed by operation arguments.
     * This is an additive C89 API; script-function values are not yet supported.
     * A callback may be shared by multiple tables through a shared metatable.
     */
    FVM_API FVM_Result fvm_meta_register(FVM_Context *, FVM_Object *metatable, const char *method,
                                         FVM_NativeFunction callback, void *userdata);
    FVM_API FVM_Result fvm_meta_register_rooted(FVM_Context *, FVM_Object *, const char *,
                                                FVM_NativeFunction, void *, FVM_Object *);
    /* Index lookup, write interception, and iterator use registered callbacks.
     * Iteration: return FVM_VALUE_UNDEFINED to signal end; otherwise return a
     * value and optionally populate key via fvm_meta_iterate's out_key.
     */
    FVM_API FVM_Result fvm_meta_iterate(FVM_Context *, FVM_Object *, unsigned long invocation,
                                        FVM_Value *out_key, FVM_Value *out_value);
    /* Attachment to a primitive belongs to that value and follows value copies.
     * Attachment to an object belongs to its shared identity. Clearing a value
     * with fvm_value_* clears primitive metadata. Metatables must belong to ctx. */
    FVM_API FVM_Result fvm_value_set_metatable(FVM_Context *, FVM_Value *, FVM_Object *);
    FVM_API FVM_Result fvm_value_get_metatable(FVM_Context *, const FVM_Value *, FVM_Object **);
    /* Invoke a registered hook. Arguments are [owner value, defining metatable,
     * operation arguments...]. Missing hooks return FVM_NOT_FOUND. */
    FVM_API FVM_Result fvm_value_meta_invoke(FVM_Context *, const FVM_Value *, const char *,
                                             unsigned int, const FVM_Value *, FVM_Value *);
    FVM_API FVM_Result fvm_value_get_property(FVM_Context *, const FVM_Value *, const char *,
                                              FVM_Value *);
    FVM_API FVM_Result fvm_value_set_property(FVM_Context *, const FVM_Value *, const char *,
                                              const FVM_Value *);
    /* Explicit finalization of an owned table, at most once.  The object remains
     * allocated until a later tracing collection or context teardown. */
    FVM_API FVM_Result fvm_meta_finalize(FVM_Context *, FVM_Object *);
#ifdef __cplusplus
}
#endif
#endif
