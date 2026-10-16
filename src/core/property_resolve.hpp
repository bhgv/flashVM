/* FlashVM: raw legacy resolver discovery for missing loaded properties. */
#ifndef FVM_CORE_PROPERTY_RESOLVE_HPP
#define FVM_CORE_PROPERTY_RESOLVE_HPP /* Resolver helper guard. */
#include "runtime.hpp"
/* [GC:] Return NOT_FOUND without an Object resolver, otherwise retain lookup and receiver through invocation. */
FVM_Result fvm_property_resolve(FVM_Context *, FVM_Object *, FVM_Object *,
                                const fvm_detail::string &, FVM_Value *);
#endif
