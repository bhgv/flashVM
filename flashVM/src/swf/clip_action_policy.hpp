/* FlashVM: shared strict file-version policy for ClipAction parser and delivery. */
#ifndef FVM_SWF_CLIP_ACTION_POLICY_HPP
#define FVM_SWF_CLIP_ACTION_POLICY_HPP /* Event-mask policy guard. */
#include "clip_action_constants.h"


/* Select the strict file-version mask, independently of the ambient receiver context. */
inline unsigned long fvm_swf_clip_action_mask(unsigned int version)
{
    return !version                                            ? FVM_SWF_CLIP_ACTION_KNOWN_MASK
           : version < FVM_SWF_CLIP_ACTION_MIN_VERSION         ? 0
           : version < FVM_SWF_CLIP_ACTION_SHORT_FLAGS_VERSION ? FVM_SWF_CLIP_ACTION_V5_MASK
           : version < FVM_SWF_CLIP_ACTION_CONSTRUCT_VERSION   ? FVM_SWF_CLIP_ACTION_V6_MASK
                                                               : FVM_SWF_CLIP_ACTION_KNOWN_MASK;
}
#endif
