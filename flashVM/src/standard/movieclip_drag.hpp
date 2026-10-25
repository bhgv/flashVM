/* FlashVM: native script adapters for context-wide dragging. */
#ifndef FVM_MOVIECLIP_DRAG_HPP
#define FVM_MOVIECLIP_DRAG_HPP /* Native drag callback declarations. */
#include "../core/runtime.hpp"


/* Prepare versioned script drag options through the shared C value service. */
FVM_Result fvm_movieclip_start_drag(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* End context-wide dragging regardless of which native instance receives the call. */
FVM_Result fvm_movieclip_stop_drag(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
#endif
