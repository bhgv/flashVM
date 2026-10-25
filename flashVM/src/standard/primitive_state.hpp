/* FlashVM: Built-in ActionScript objects: primitive state.
 * File: src/standard/primitive_state.hpp.
 */

#ifndef FVM_STANDARD_PRIMITIVE_STATE_HPP
/* Define FVM STANDARD PRIMITIVE STATE HPP constant or compilation switch. */
#define FVM_STANDARD_PRIMITIVE_STATE_HPP
#include "flashvm/value.h"


/* Implement to primitive for this subsystem. */
FVM_Result fvm_to_primitive(FVM_Context *, const FVM_Value *, bool string_hint, FVM_Value *);


/* Atomically activate standard primitive prototypes before publishing decoded SWF state. */
FVM_Result fvm_primitive_activate_loaded(FVM_Context *);
#endif
