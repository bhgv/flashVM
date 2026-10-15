/* FlashVM: internal prepared-activation dispatch, shared with the interpreter call path. */
#ifndef FVM_NATIVE_BODY_HPP
#define FVM_NATIVE_BODY_HPP /* Private dispatch guard. */
#include "../../core/runtime.hpp"
/* [GC:] Invoke a snapshotted generated body inside script this/target scopes. */
FVM_Result fvm_exec_native_body(FVM_Context *, const FVM_ScriptBodyEntry &, const FVM_ScriptFrame &,
                                FVM_Value *, const fvm_detail::vector<fvm_detail::string> *pool = 0);
#endif
