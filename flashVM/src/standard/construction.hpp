/* FlashVM: internal value-returning AVM1 construction boundary. */
#ifndef FVM_STANDARD_CONSTRUCTION_HPP
#define FVM_STANDARD_CONSTRUCTION_HPP /* Private construction interface guard. */
#include "../core/runtime.hpp"


/* [GC:] Construct a rooted AVM1 value; public object-only C construction remains strict. */
FVM_Result fvm_function_construct_value(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *);
#endif
