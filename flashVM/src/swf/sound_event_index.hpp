/* FlashVM: private canonical root-frame sound index, independent of managed timeline metadata. */
#ifndef FVM_SWF_SOUND_EVENT_INDEX_HPP
#define FVM_SWF_SOUND_EVENT_INDEX_HPP /* Internal C++98 event-index guard. */
#include "../core/runtime.hpp"


/* Return native global ordinals for one canonical frame; errors preserve the output vector.
 * [GC:] Values are copied indices, not managed pointers. The caller owns context/source lifetime.
 * Work shares the caller's scope; no callbacks, decoding, device output or retained index cache. */
FVM_Result fvm_swf_sound_frame_ordinals(FVM_Context *, unsigned int frame, fvm_detail::vector<size_t> &out);
#endif
