/* FlashVM: explicit point-conversion roots, independent of native argument positions. */
#ifndef FVM_CLIP_POINT_CONSTANTS_H
#define FVM_CLIP_POINT_CONSTANTS_H  /* Point service root layout guard. */
#define FVM_CLIP_POINT_CLIP_ROOT 0U /* [GC:] Clip and its physical ancestor graph. */
#define FVM_CLIP_POINT_VALUE_ROOT                                                                  \
    1U                               /* [GC:] Point retained through coordinate/geometry getters.  \
                                      */
#define FVM_CLIP_POINT_ROOT_COUNT 2U /* [GC:] Complete point-conversion root layout. */
/* Canonical field names and their counted UCS2 lengths for pre-SWF7 Flash case folding. */
#define FVM_CLIP_POINT_X_NAME "x"
#define FVM_CLIP_POINT_Y_NAME "y"
#define FVM_CLIP_POINT_FIELD_UNITS 1U
#endif
