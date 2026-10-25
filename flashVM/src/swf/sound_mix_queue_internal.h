/* Private C++98 pairing check for stream/composition operations; no public ownership API. */
#ifndef FVM_SOUND_MIX_QUEUE_INTERNAL_H
#define FVM_SOUND_MIX_QUEUE_INTERNAL_H /* Guard the exact adapted-backend identity check. */
#include "flashvm/swf_sound_mix_queue.h"


/* Verify the complete live adapted callback table without polling or changing either owner. */
bool fvm_swf_sound_mix_queue_matches(const FVM_SWFSoundMixQueue *, const FVM_AudioQueueBackend *);


/* Return the successful adaptive write's admission observation without polling after acceptance.
 * Native stream caller already verifies an active equal epoch and holds its own busy guard.
 * Optional clock is untouched on failure; all public adaptive-tail restrictions still apply. */
FVM_Result fvm_swf_sound_mix_queue_pump_tail_observed(
    FVM_SWFSoundMixQueue *, size_t, size_t, size_t *, FVM_SWFSoundMixQueueClock *, bool finite);


/* Copy the last accepted/observed active clock without a callback or fallible operation.
 * Only paired callers with validated live/exclusive owners and active epoch may use this. */
void fvm_swf_sound_mix_queue_last_clock(const FVM_SWFSoundMixQueue *, FVM_SWFSoundMixQueueClock *);
#endif
