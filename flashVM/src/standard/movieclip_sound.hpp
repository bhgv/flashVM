/* FlashVM: actual frame-entry sound delivery, kept separate from visual seek reconstruction. */
#ifndef FVM_MOVIECLIP_SOUND_HPP
#define FVM_MOVIECLIP_SOUND_HPP /* Guard the internal frame-to-host bridge. */
#include "../core/runtime.hpp"


/* [GC:] The caller's native-call barrier roots the clip/frame graph during synchronous delivery. */
FVM_Result fvm_movieclip_sound_run(FVM_Context *, FVM_Object *, FVM_Object *frames, unsigned long frame);
#endif
