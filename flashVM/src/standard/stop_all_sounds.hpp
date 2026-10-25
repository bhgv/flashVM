/* FlashVM: global AS1/AS2 stopAllSounds bridge to context audio registrations. */
#ifndef FVM_STOP_ALL_SOUNDS_HPP
#define FVM_STOP_ALL_SOUNDS_HPP /* Guard the stable native callback declaration. */
#include "../core/runtime.hpp"


/* Ignore script operands and stop the current context's active host audio outputs. */
FVM_Result fvm_stop_all_sounds(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
#endif
