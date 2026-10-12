#ifndef FVM_ARRAY_GENERIC_HPP
#define FVM_ARRAY_GENERIC_HPP
#include "../core/runtime.hpp"
bool fvm_array_needs_generic(FVM_Object *);
FVM_Result fvm_array_generic(FVM_Context *, FVM_Object *, const char *, unsigned int,
                             const FVM_Value *, FVM_Value *, FVM_NativeFunction);
#endif
