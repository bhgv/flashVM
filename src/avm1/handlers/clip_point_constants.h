/* FlashVM: explicit point-conversion roots, independent of native argument positions. */
#ifndef FVM_CLIP_POINT_CONSTANTS_H
#define FVM_CLIP_POINT_CONSTANTS_H  /* Point service root layout guard. */
#define FVM_CLIP_POINT_CLIP_ROOT 0U /* [GC:] Clip and its physical ancestor graph. */
#define FVM_CLIP_POINT_VALUE_ROOT                                                                  \
    1U                               /* [GC:] Point retained through coordinate/geometry getters.  \
                                      */
#define FVM_CLIP_POINT_ROOT_COUNT 2U /* [GC:] Complete point-conversion root layout. */
/* Loaded point coordinates first enter the SWF signed 32-bit twip representation.
 * Floating bounds avoid implementation-defined integer casts and overflow on 32-bit hosts. */
#define FVM_CLIP_POINT_TWIPS_PER_PIXEL 20.0
#define FVM_CLIP_POINT_MAX_TWIPS 2147483647.0
#define FVM_CLIP_POINT_MIN_TWIPS (-2147483648.0)
#endif
