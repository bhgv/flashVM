/* FlashVM: stable graph identities for the generic broadcaster's native callbacks. */
#ifndef FVM_STANDARD_BROADCASTER_HPP
#define FVM_STANDARD_BROADCASTER_HPP /* Private C++98 graph codec declarations. */
#include "../core/runtime.hpp"


/* Return a stable graph identifier, or zero for an unrelated native callback. */
unsigned long fvm_broadcaster_callback_id(FVM_NativeFunction);


/* Restore a supported graph identity, or null for an unrelated/unknown identifier. */
FVM_NativeFunction fvm_broadcaster_callback_from_id(unsigned long);
#endif
