/* FlashVM: Public embedding API declarations: metamethod.
 * File: include/flashvm/metamethod.h.
 */

#ifndef FLASHVM_METAMETHOD_H
/* Define FLASHVM METAMETHOD H constant or compilation switch. */
#define FLASHVM_METAMETHOD_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Native metamethod callbacks receive the context and positional values:
     * [0] owner table, [1] defining metatable, followed by operation arguments.
     * This is an additive C89 API; first-class functions stored in metatable
     * properties use the same positional argument convention.
     * A callback may be shared by multiple tables through a shared metatable.
     */
    /* Register meta. */
    FVM_API FVM_Result fvm_meta_register(FVM_Context *, FVM_Object *metatable, const char *method,
                                         FVM_NativeFunction callback, void *userdata);
    /* [GC:] Register meta rooted. */
    FVM_API FVM_Result fvm_meta_register_rooted(FVM_Context *, FVM_Object *, const char *,
                                                FVM_NativeFunction, void *, FVM_Object *);
    /* Index lookup, write interception, and iterator use registered callbacks.
     * Iteration: return FVM_VALUE_UNDEFINED to signal end; otherwise return a
     * value and optionally populate key via fvm_meta_iterate's out_key.
     */
    /* Implement meta iterate for this subsystem. */
    FVM_API FVM_Result fvm_meta_iterate(FVM_Context *, FVM_Object *, unsigned long invocation,
                                        FVM_Value *out_key, FVM_Value *out_value);
    /* Attachment to a primitive belongs to that value and follows value copies.
     * Attachment to an object belongs to its shared identity. Clearing a value
     * with fvm_value_* clears primitive metadata. Metatables must belong to ctx. */
    /* Update value metatable. */
    FVM_API FVM_Result fvm_value_set_metatable(FVM_Context *, FVM_Value *, FVM_Object *);
    /* Read value metatable. */
    FVM_API FVM_Result fvm_value_get_metatable(FVM_Context *, const FVM_Value *, FVM_Object **);
    /* Invoke a registered hook. Arguments are [owner value, defining metatable,
     * operation arguments...]. Missing hooks return FVM_NOT_FOUND. */
    /* Invoke value meta. */
    FVM_API FVM_Result fvm_value_meta_invoke(FVM_Context *, const FVM_Value *, const char *,
                                             unsigned int, const FVM_Value *, FVM_Value *);
    /* Read value property. */
    FVM_API FVM_Result fvm_value_get_property(FVM_Context *, const FVM_Value *, const char *,
                                              FVM_Value *);
    /* Update value property. */
    FVM_API FVM_Result fvm_value_set_property(FVM_Context *, const FVM_Value *, const char *,
                                              const FVM_Value *);
    /* Read a property through a counted WTF-8 name, including primitive boxing. */
    FVM_API FVM_Result fvm_value_get_property_bytes(FVM_Context *, const FVM_Value *, const char *,
                                                    size_t, FVM_Value *);
    /* Assign through a counted name, including primitive metatable hooks. */
    FVM_API FVM_Result fvm_value_set_property_bytes(FVM_Context *, const FVM_Value *, const char *,
                                                    size_t, const FVM_Value *);
    /* Explicit finalization of an owned table, at most once.  The object remains
     * allocated until a later tracing collection or context teardown. */
    /* Implement meta finalize for this subsystem. */
    /* [GC:] Вызываем финализатор; он может сделать объект снова достижимым. */
    FVM_API FVM_Result fvm_meta_finalize(FVM_Context *, FVM_Object *);
#ifdef __cplusplus
}
#endif
#endif
