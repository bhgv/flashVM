/* FlashVM: explicit target-removal operands retained through path callbacks. */
#ifndef FVM_REMOVE_TARGET_CONSTANTS_H
#define FVM_REMOVE_TARGET_CONSTANTS_H /* Target-removal service layout guard. */
#define FVM_REMOVE_TARGET_START_ROOT 0U /* [GC:] Original path-start clip. */
#define FVM_REMOVE_TARGET_INPUT_ROOT 1U /* [GC:] Original target value. */
#define FVM_REMOVE_TARGET_CLIP_ROOT 2U /* [GC:] Resolved native instance. */
#define FVM_REMOVE_TARGET_ROOT_COUNT 3U /* [GC:] Complete target-removal temporary layout. */
#endif
