/* FlashVM: bounds for one selected scene's retained global input-event broadcast. */
#ifndef FVM_STANDARD_CLIP_INPUT_CONSTANTS_H
#define FVM_STANDARD_CLIP_INPUT_CONSTANTS_H /* Scene-input policy guard. */
#define FVM_CLIP_INPUT_KEY_PRESS_MIN_VERSION                                                                           \
    6U /* First source version with retained KeyPress selectors, independent of named methods. */
#define FVM_CLIP_INPUT_METHOD_MIN_VERSION                                                                              \
    6U /* Named MovieClip input methods start at SWF6; host version zero enables them. */
#define FVM_CLIP_INPUT_MAX_DEPTH 64U          /* Maximum selected-root descendant depth. */
#define FVM_CLIP_INPUT_MAX_NODES 65536UL      /* Count all inspected native nodes, including static art. */
#define FVM_CLIP_INPUT_MAX_CHILDREN 65536UL   /* Bound the child-depth sorting workspace per node. */
#define FVM_CLIP_INPUT_MAX_RECIPIENTS 65536UL /* Bound one prepared scene-recipient batch. */
#define FVM_CLIP_INPUT_EVENT_MASK 0x000f8UL   /* Mouse move/down/up and key down/up, excluding targeted events. */
#endif
