#ifndef FLASHVM_DEBUG_H
#define FLASHVM_DEBUG_H
#include "table.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Snapshot-style inspection: borrowed handles; only call on the VM thread. */
FVM_API FVM_Result fvm_debug_object_count(FVM_Context *, size_t *);
FVM_API FVM_Result fvm_debug_object_at(FVM_Context *, size_t, FVM_Object **);
FVM_API FVM_Result fvm_debug_native_function_count(FVM_Context *, size_t *);
FVM_API FVM_Result fvm_debug_native_function_at(FVM_Context *, size_t, const char **);
FVM_API FVM_Result fvm_debug_script_function_count(FVM_Context *, size_t *);
FVM_API FVM_Result fvm_debug_script_function_at(FVM_Context *, size_t, const char **);
#ifdef __cplusplus
}
#endif
#endif
