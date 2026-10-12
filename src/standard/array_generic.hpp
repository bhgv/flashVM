/* FlashVM: Built-in ActionScript objects: array generic.
 * File: src/standard/array_generic.hpp.
 */

#ifndef FVM_ARRAY_GENERIC_HPP
/* Define FVM ARRAY GENERIC HPP constant or compilation switch. */
#define FVM_ARRAY_GENERIC_HPP
#include "../core/runtime.hpp"
/* Implement array needs generic for this subsystem. */
bool fvm_array_needs_generic(FVM_Object *);

/* Implement array generic for this subsystem. */
FVM_Result fvm_array_generic(FVM_Context *, FVM_Object *, const char *, unsigned int,
                             const FVM_Value *, FVM_Value *, FVM_NativeFunction);
#endif
