/* Private C++98 pairing check for stream/composition operations; no public ownership API. */
#ifndef FVM_SOUND_MIX_QUEUE_INTERNAL_H
#define FVM_SOUND_MIX_QUEUE_INTERNAL_H /* Guard the exact adapted-backend identity check. */
#include "flashvm/swf_sound_mix_queue.h"
/* Verify the complete live adapted callback table without polling or changing either owner. */
bool fvm_swf_sound_mix_queue_matches(const FVM_SWFSoundMixQueue *, const FVM_AudioQueueBackend *);
#endif
