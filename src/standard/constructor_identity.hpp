/* FlashVM: forced own constructor identities with atomic metadata publication. */
#ifndef FVM_STANDARD_CONSTRUCTOR_IDENTITY_HPP
#define FVM_STANDARD_CONSTRUCTOR_IDENTITY_HPP /* Private constructor-publication guard. */
#include "../core/runtime.hpp"
/* [GC:] Define hidden identities; explicit movie version overrides ambient SWF, zero selects loaded context. */
FVM_Result fvm_function_define_constructor(FVM_Context *, FVM_Object *receiver, FVM_Object *callee,
                                             unsigned int movie_version = 0);
#endif
