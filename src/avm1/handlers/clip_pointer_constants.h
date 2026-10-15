/* FlashVM: explicit roots for context-pointer coordinate projection. */
#ifndef FVM_CLIP_POINTER_CONSTANTS_H
#define FVM_CLIP_POINTER_CONSTANTS_H      /* Local-pointer root layout guard. */
#define FVM_CLIP_POINTER_RECEIVER_ROOT 0U /* [GC:] Native clip whose matrix is queried. */
#define FVM_CLIP_POINTER_ROOT_COUNT 1U    /* [GC:] Complete projection temporary layout. */
/* Binary32 twip-to-device-pixel scale for the current unit-ratio scene. */
#define FVM_CLIP_POINTER_PIXEL_SCALE 0.05f
#endif
