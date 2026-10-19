/* FlashVM: native Mouse listener methods shared with explicit namespace installation. */
#ifndef FVM_STANDARD_MOUSE_LISTENERS_HPP
#define FVM_STANDARD_MOUSE_LISTENERS_HPP /* Guard private listener callback declarations. */
#include "../core/runtime.hpp"
/* Change requested visibility and return the previous logical 0/1 as a Number (SWF5+). */
FVM_Result fvm_mouse_hide(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                        FVM_Value *, void *);
/* Restore requested visibility; argument values are ignored without coercion. */
FVM_Result fvm_mouse_show(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                        FVM_Value *, void *);
/* Register one object identity; script-invalid arguments leave registrations unchanged. */
FVM_Result fvm_mouse_add_listener(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                FVM_Value *, void *);
/* Return a Boolean indicating whether an object was removed. */
FVM_Result fvm_mouse_remove_listener(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                   FVM_Value *, void *);
/* [GC:] Return a fresh Array snapshot, retaining the current registered listener objects. */
FVM_Result fvm_mouse_listener_array(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                  FVM_Value *, void *);
#endif
