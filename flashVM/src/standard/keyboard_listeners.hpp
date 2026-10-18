/* FlashVM: native Key listener methods shared with explicit namespace installation. */
#ifndef FVM_STANDARD_KEYBOARD_LISTENERS_HPP
#define FVM_STANDARD_KEYBOARD_LISTENERS_HPP /* Guard private listener callback declarations. */
#include "../core/runtime.hpp"
/* Register one object identity; script-invalid arguments leave registrations unchanged. */
FVM_Result fvm_key_add_listener(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                FVM_Value *, void *);
/* Return a Boolean indicating whether an object was removed. */
FVM_Result fvm_key_remove_listener(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                   FVM_Value *, void *);
/* [GC:] Return a fresh Array snapshot, retaining the current registered listener objects. */
FVM_Result fvm_key_listener_array(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                  FVM_Value *, void *);
#endif
