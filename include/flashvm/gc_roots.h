#ifndef FLASHVM_GC_ROOTS_H
#define FLASHVM_GC_ROOTS_H
#include "flashvm/value.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Temporary native roots. Scopes are strictly LIFO and context-local.
 * Values are copied, not referenced. Call end before destroying the context.
 * Never share a scope across threads. */
typedef struct FVM_GCRootScope FVM_GCRootScope;
FVM_API FVM_Result fvm_gc_roots_begin(FVM_Context *, FVM_GCRootScope **);
FVM_API FVM_Result fvm_gc_roots_add(FVM_GCRootScope *, const FVM_Value *);
FVM_API FVM_Result fvm_gc_roots_end(FVM_GCRootScope *);
#ifdef __cplusplus
}
#endif
#endif
