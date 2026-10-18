/* FlashVM: forced own constructor identities with atomic metadata publication. */
#ifndef FVM_STANDARD_CONSTRUCTOR_IDENTITY_HPP
#define FVM_STANDARD_CONSTRUCTOR_IDENTITY_HPP /* Private constructor-publication guard. */
#include "../core/runtime.hpp"
/* [GC:] Define hidden identities; explicit movie version overrides ambient SWF, zero selects loaded context. */
FVM_Result fvm_function_define_constructor(FVM_Context *, FVM_Object *receiver, FVM_Object *callee,
                                             unsigned int movie_version = 0);
/* [GC:] Allocate a loaded built-in receiver with stored prototype and constructor identities. */
FVM_Result fvm_function_allocate_instance(FVM_Context *, FVM_Object *callee,
                                            int resource_kind, FVM_Object **out);
#endif
