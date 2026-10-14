/* FlashVM: private variable-path phase with qualified callable receiver metadata. */
#ifndef FVM_VARIABLE_PATH_HPP
#define FVM_VARIABLE_PATH_HPP /* Private qualified-resolution bridge guard. */
#include "flashvm/execution.h"
/* [GC:] Qualified values return their owner; bare slash values request the default receiver. */
FVM_Result fvm_avm1_read_variable_path(FVM_Context *, FVM_Object *scope, FVM_Object *target,
                                       FVM_Object *receiver, const FVM_Value *name, FVM_Value *out,
                                       FVM_Object **owner, FVM_Object **source_owner, int *handled,
                                       int *present);
#endif
