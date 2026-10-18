/* FlashVM: retained operands for intrinsic sibling construction. */
#ifndef FVM_DUPLICATE_CLIP_CONSTANTS_H
#define FVM_DUPLICATE_CLIP_CONSTANTS_H /* Construction root layout guard. */
#define FVM_DUPLICATE_CLIP_RECEIVER_ROOT 0U /* [GC:] Original native instance. */
#define FVM_DUPLICATE_CLIP_NAME_ROOT 1U /* [GC:] Coercible name. */
#define FVM_DUPLICATE_CLIP_DEPTH_ROOT 2U /* [GC:] Coercible public depth. */
#define FVM_DUPLICATE_CLIP_INITIAL_ROOT 3U /* [GC:] Optional initialization value. */
#define FVM_DUPLICATE_CLIP_RESULT_ROOT 4U /* [GC:] Unpublished result value. */
#define FVM_DUPLICATE_CLIP_ROOT_COUNT 5U /* [GC:] Complete construction temporary layout. */
#endif
