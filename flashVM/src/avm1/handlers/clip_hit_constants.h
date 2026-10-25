/* FlashVM: rooted operands for intrinsic world-space geometry. */
#ifndef FVM_CLIP_HIT_CONSTANTS_H
#define FVM_CLIP_HIT_CONSTANTS_H          /* Geometry root layout guard. */
#define FVM_CLIP_HIT_RECEIVER_ROOT 0U     /* [GC:] Tested native clip. */
#define FVM_CLIP_HIT_TARGET_ROOT 1U       /* [GC:] Other native clip for bounds overlap. */
#define FVM_CLIP_HIT_POINT_ROOT_COUNT 1U  /* [GC:] Point test needs only its receiver. */
#define FVM_CLIP_HIT_OBJECT_ROOT_COUNT 2U /* [GC:] Object test retains both clips. */
#endif
