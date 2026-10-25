/* FlashVM: raw legacy resolver discovery for missing loaded properties. */
#ifndef FVM_CORE_PROPERTY_RESOLVE_HPP
#define FVM_CORE_PROPERTY_RESOLVE_HPP /* Resolver helper guard. */
#include "runtime.hpp"

/* [GC:] Report Object-resolver presence separately from callback status; retain lookup/receiver through invocation. */
FVM_Result
fvm_property_resolve(FVM_Context *, FVM_Object *, FVM_Object *, const fvm_detail::string &, FVM_Value *, bool *handled);
#endif
