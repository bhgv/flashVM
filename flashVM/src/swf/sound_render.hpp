/* FlashVM: prepare one bounded StartSound voice without entering host callbacks. */
#ifndef FVM_SWF_SOUND_RENDER_HPP
#define FVM_SWF_SOUND_RENDER_HPP /* Guard the private copied-voice renderer. */
#include "../core/runtime.hpp"
#include "flashvm/swf_sound.h"


/* Validate styles, decode once and render interleaved S16 into native-owned storage. */
FVM_Result fvm_swf_sound_render(
    FVM_Context *, size_t, const FVM_SWFSoundEvent &, const FVM_SWFSoundInfo &, fvm_detail::vector<short> &);
#endif
