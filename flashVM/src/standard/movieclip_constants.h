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
/* Maximum independent DoAction blocks retained by one frame snapshot. */
#define FVM_MOVIECLIP_MAX_FRAME_BLOCKS 65536UL
/* Per-library initializer table bound, independent of ordinary frame action limits. */
#define FVM_MOVIECLIP_MAX_INITIALIZER_BLOCKS 65536UL
/* Nested ActionCall depth, independent of display-tree recursion. */
#define FVM_MOVIECLIP_CALL_FRAME_MAX_DEPTH 64U
/* Maximum descendant depth visited by one native timeline traversal. */
#define FVM_MOVIECLIP_ADVANCE_MAX_DEPTH 64U
#define FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN 65536UL /* Bound each post-handler native child snapshot. */
/* Independent raster recursion and per-node child-snapshot bounds. */
#define FVM_MOVIECLIP_RENDER_MAX_DEPTH 64U
#define FVM_MOVIECLIP_RENDER_MAX_CHILDREN 65536UL
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
/* Intrinsic dimension setters store scale as percent rather than unit ratios. */
#define FVM_MOVIECLIP_DIMENSION_PERCENT 100.0
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
/* Stable native _lockroot getter and setter IDs; existing callbacks are never renumbered. */
#define FVM_MOVIECLIP_CALLBACK_LOCKROOT_GET 1006UL
#define FVM_MOVIECLIP_CALLBACK_LOCKROOT_SET 1007UL
#define FVM_MOVIECLIP_CALLBACK_XMOUSE_GET 1008UL /* Context-local pointer X getter; stable graph ID. */
#define FVM_MOVIECLIP_CALLBACK_YMOUSE_GET 1009UL /* Context-local pointer Y getter; stable graph ID. */
#define FVM_MOVIECLIP_CALLBACK_QUALITY_GET 1200UL /* Stable quality accessor identity. */
#define FVM_MOVIECLIP_CALLBACK_QUALITY_SET 1201UL /* Stable quality accessor identity. */
#define FVM_MOVIECLIP_CALLBACK_HIGHQUALITY_GET 1202UL /* Stable quality accessor identity. */
#define FVM_MOVIECLIP_CALLBACK_HIGHQUALITY_SET 1203UL /* Stable quality accessor identity. */
/* Ordered method callbacks occupy consecutive IDs beginning here. */
#define FVM_MOVIECLIP_CALLBACK_METHOD_BASE 1010UL
#define FVM_MOVIECLIP_CLASS_PROTOTYPE_KEY                                                          \
    "$classPrototype" /* Private Boolean marking the builtin prototype, not a live display         \
                         instance. */
/* Native instance has not undergone a detach/replacement transaction. */
#define FVM_MOVIECLIP_ATTACHED_STATE 0U
/* Native instance was explicitly detached or displaced from its parent slot. */
#define FVM_MOVIECLIP_DETACHED_STATE 1U
/* First public depth removable through the dynamic-clip execution policy. */
#define FVM_MOVIECLIP_MIN_DYNAMIC_DEPTH 0L
/* Guard against cyclic/corrupt native parent chains during removal eligibility checks. */
#define FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS 256U
/* Bound native AVM1 root ancestry independently from object prototype lookup. */
#define FVM_MOVIECLIP_ROOT_MAX_ANCESTORS 1024U
/* Native hitTest argument layouts; object and point forms retain distinct meanings. */
#define FVM_MOVIECLIP_HIT_OBJECT_ARGUMENTS 1U
#define FVM_MOVIECLIP_HIT_OBJECT_INDEX 0U /* Explicit display-object operand. */
#define FVM_MOVIECLIP_HIT_POINT_ARGUMENTS 2U
#define FVM_MOVIECLIP_HIT_X_INDEX 0U
#define FVM_MOVIECLIP_HIT_Y_INDEX 1U
#define FVM_MOVIECLIP_HIT_SHAPE_INDEX 2U
/* Bound unlocked root ancestry when mapping loaded hitTest points to world space. */
#define FVM_MOVIECLIP_HIT_ROOT_MAX_ANCESTORS 1024U
#define FVM_MOVIECLIP_GEOMETRY_MAX_DEPTH 64U /* Maximum recursive native bounds/paint descent. */
/* Native sibling construction uses separate roles from hit-test operands. */
#define FVM_MOVIECLIP_DUPLICATE_REQUIRED_ARGUMENTS 2U /* Name and public depth are required. */
#define FVM_MOVIECLIP_DUPLICATE_NAME_INDEX 0U /* Coercible instance name. */
#define FVM_MOVIECLIP_DUPLICATE_DEPTH_INDEX 1U /* Coercible native public depth. */
#define FVM_MOVIECLIP_DUPLICATE_INITIAL_INDEX 2U /* Optional initialization value. */
#define FVM_MOVIECLIP_DUPLICATE_ARGUMENT_CAPACITY 3U /* Complete native adapter operand array. */
/* Absolute AVM1 clone depths use a separate bias from SWF placement depth encoding. */
#define FVM_MOVIECLIP_DUPLICATE_DEPTH_BIAS 16384L
/* Historical clone upper bound: 2^31 minus 16777220, matching AVM_MAX_DEPTH. */
#define FVM_MOVIECLIP_DUPLICATE_MAX_ABSOLUTE_DEPTH 2130706428L
/* Maximum physical ancestor transforms composed by world geometry queries. */
#define FVM_MOVIECLIP_MATRIX_MAX_ANCESTORS 1024U

#define FVM_MOVIECLIP_VIEW_COMPONENTS 6U /* Finite affine viewport scalar count. */
#endif
