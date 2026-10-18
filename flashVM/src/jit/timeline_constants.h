/* FlashVM: native dynamic timeline policy, independent from encoded Action flags. */
#ifndef FVM_JIT_TIMELINE_CONSTANTS_H
#define FVM_JIT_TIMELINE_CONSTANTS_H /* Native frame policy guard. */
#define FVM_JIT_FRAME_PLAY 1U /* Resume playback after a dynamic seek. */
#define FVM_JIT_FRAME_WITH_BIAS 2U /* Apply the stored scene offset to numeric requests. */
#define FVM_JIT_FRAME_BIAS_SHIFT 2U /* Bit position of the bounded unsigned scene offset. */
#endif
