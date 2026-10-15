/* FlashVM: MovieClip timeline, geometry, display units and stable callback constants. */
#ifndef FVM_MOVIECLIP_CONSTANTS_H
#define FVM_MOVIECLIP_CONSTANTS_H       /* Private MovieClip-state guard. */
#define FVM_MOVIECLIP_PLAYBACK_STOPPED 0.0 /* Detached clips stop native timeline playback. */
#define FVM_MOVIECLIP_DEFAULT_FRAMES 1U /* Empty native MovieClips expose one initial frame. */
/* UI16 timeline frame count accepted by native construction and frame metadata. */
#define FVM_MOVIECLIP_MAX_FRAMES 65535UL
/* Signed public depth range currently accepted by the native MovieClip model.
 * These are implementation bounds, not complete historical Flash depth semantics. */
#define FVM_MOVIECLIP_MIN_PUBLIC_DEPTH (-16384L)
#define FVM_MOVIECLIP_MAX_PUBLIC_DEPTH 1048575L
/* Bias encoding signed public depths as nonnegative child-table indices.
 * Legacy snapshots omit the bias and contain only nonnegative public depths. */
#define FVM_MOVIECLIP_CHILD_SLOT_BIAS 16384UL
/* SWF placement depths are offset before entering the public native depth model.
 * Kept separate from child-slot encoding even though the current values match. */
#define FVM_MOVIECLIP_SWF_DEPTH_OFFSET 16384L
/* UI16 authored placement depth, distinct from runtime/dynamic depth ranges. */
#define FVM_MOVIECLIP_MAX_AUTHORED_DEPTH 65535UL
/* Native frame-definition action-buffer limit; not a drawing or tick limit. */
#define FVM_MOVIECLIP_MAX_FRAME_ACTION_BYTES 65536UL
/* Maximum timeline steps accepted by one native tick call. */
#define FVM_MOVIECLIP_MAX_TICKS_PER_CALL 65536UL
/* Resource bounds for stored geometry commands and path groups. */
#define FVM_MOVIECLIP_MAX_DRAW_COMMANDS 65536UL
#define FVM_MOVIECLIP_MAX_DRAW_PATHS 4096UL
/* Accepted absolute draw-coordinate magnitude in pixels. */
#define FVM_MOVIECLIP_MAX_DRAW_COORDINATE 1000000.0
/* Accepted absolute point magnitude for shape hit testing, in pixels. */
#define FVM_MOVIECLIP_MAX_HIT_COORDINATE 2000000.0
/* SWF position quantization: twenty integer twips represent one pixel. */
#define FVM_MOVIECLIP_TWIPS_PER_PIXEL 20.0
/* Degree/radian conversion and rotation normalization bounds. */
#define FVM_MOVIECLIP_DEGREES_PER_TURN 360.0
#define FVM_MOVIECLIP_DEGREES_PER_HALF_TURN 180.0
/* Stable callback identities embedded in existing graph transport; do not renumber. */
#define FVM_MOVIECLIP_CALLBACK_CONSTRUCTOR 1000UL
#define FVM_MOVIECLIP_CALLBACK_PARENT 1001UL
#define FVM_MOVIECLIP_CALLBACK_ROOT 1002UL
#define FVM_MOVIECLIP_CALLBACK_CURRENT_FRAME 1003UL
#define FVM_MOVIECLIP_CALLBACK_TOTAL_FRAMES 1004UL
#define FVM_MOVIECLIP_CALLBACK_TARGET_PATH 1005UL
/* Ordered method callbacks occupy consecutive IDs beginning here. */
#define FVM_MOVIECLIP_CALLBACK_METHOD_BASE 1010UL
#define FVM_MOVIECLIP_CLASS_PROTOTYPE_KEY                                                          \
    "$classPrototype" /* Private Boolean marking the builtin prototype, not a live display         \
                         instance. */
/* Native instance has not undergone a detach/replacement transaction. */
#define FVM_MOVIECLIP_ATTACHED_STATE 0U
/* Native instance was explicitly detached or displaced from its parent slot. */
#define FVM_MOVIECLIP_DETACHED_STATE 1U
#endif
