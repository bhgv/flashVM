#ifndef FLASHVM_METAMETHOD_H
#define FLASHVM_METAMETHOD_H
#include "value.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native metamethod callbacks receive the context and positional values:
 * [0] owner table, [1] defining metatable, followed by operation arguments.
 * This is an additive C89 API; script-function values are not yet supported.
 * A callback may be shared by multiple tables through a shared metatable.
 */
FVM_API FVM_Result fvm_meta_register(FVM_Context *, FVM_Object *metatable,
    const char *method, FVM_NativeFunction callback, void *userdata);
/* Index lookup, write interception, and iterator use registered callbacks.
 * Iteration: return FVM_VALUE_UNDEFINED to signal end; otherwise return a
 * value and optionally populate key via fvm_meta_iterate's out_key.
 */
FVM_API FVM_Result fvm_meta_iterate(FVM_Context *, FVM_Object *,
    unsigned long invocation, FVM_Value *out_key, FVM_Value *out_value);
/* Explicit finalization of an owned table, at most once.  The object remains
 * allocated until context teardown; this is NOT a tracing GC. */
FVM_API FVM_Result fvm_meta_finalize(FVM_Context *, FVM_Object *);
#ifdef __cplusplus
}
#endif
#endif
