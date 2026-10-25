/* FlashVM: stable native adapters for clip-local context pointer coordinates. */
#ifndef FVM_MOVIECLIP_POINTER_HPP
#define FVM_MOVIECLIP_POINTER_HPP /* Native pointer adapter declarations. */
#include "../core/runtime.hpp"


/* Return local pointer X, or undefined for the class prototype. */
FVM_Result fvm_movieclip_xmouse_get(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* Return local pointer Y, or undefined for the class prototype. */
FVM_Result fvm_movieclip_ymouse_get(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
#endif
