#ifndef FLASHVM_GC_H
#define FLASHVM_GC_H
#include "flashvm/value.h"
#ifdef __cplusplus
extern "C" {
#endif
/* GC is context-local; never call from another thread. */
#define FVM_GC_SUSPENDED 101
#define FVM_GC_FULL 102
/* -100..100: collection frequency; 0: normal. */
FVM_API FVM_Result fvm_gc_mode(FVM_Context *, int mode);
FVM_API FVM_Result fvm_gc_state(FVM_Context *, int *mode);
/* Explicit full mark/sweep; counts are optional. */
FVM_API FVM_Result fvm_gc_collect(FVM_Context *, unsigned long *reclaimed);
/* Pin an object while its pointer is held by native code. */
FVM_API FVM_Result fvm_gc_pin(FVM_Context *, FVM_Object *);
FVM_API FVM_Result fvm_gc_pin_count(FVM_Context *, FVM_Object *, unsigned long *count);
FVM_API FVM_Result fvm_gc_unpin(FVM_Context *, FVM_Object *);
FVM_API FVM_Result fvm_gc_object_count(FVM_Context *, unsigned long *);
/* Number of object allocations since the last successful collection. */
FVM_API FVM_Result fvm_gc_pending_allocations(FVM_Context *, unsigned long *count);
/* Query the allocation threshold for automatic collection at a safe point. */
FVM_API FVM_Result fvm_gc_threshold(FVM_Context *, unsigned long *threshold);
/* Snapshot of collector state. This is a diagnostic API, not a memory-size
   accounting API. struct_size permits future extension of the C89 ABI. */
typedef struct FVM_GCStats {
    unsigned int struct_size;
    unsigned long live_objects;
    unsigned long pinned_objects;
    unsigned long pending_allocations;
    unsigned long threshold;
    int mode;
    int collecting;
    unsigned int native_depth;
} FVM_GCStats;
FVM_API FVM_Result fvm_gc_stats(FVM_Context *, FVM_GCStats *);
/* Call at safe points. Never collect inside native callbacks. */
FVM_API FVM_Result fvm_gc_step(FVM_Context *, unsigned long *reclaimed);
#ifdef __cplusplus
}
#endif
#endif
