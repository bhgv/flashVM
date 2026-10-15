/* FlashVM: intrinsic root-lock callbacks and private-state query. */
#ifndef FVM_MOVIECLIP_LOCKROOT_HPP
#define FVM_MOVIECLIP_LOCKROOT_HPP /* Root-lock native bridge guard. */
#include "../core/runtime.hpp"
/* Read the native Boolean flag without invoking replaceable script properties. */
FVM_Result fvm_movieclip_root_locked(FVM_Context *, FVM_Object *, bool *out);
/* Return the intrinsic lock flag for an instance, or undefined for the class prototype. */
FVM_Result fvm_movieclip_lockroot_get(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                      FVM_Value *, void *);
/* Convert an assigned value through the common rooted C service. */
FVM_Result fvm_movieclip_lockroot_set(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                      FVM_Value *, void *);
#endif
