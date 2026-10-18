/* FlashVM: temporary ownership roots for forced display detachment. */
#ifndef FVM_DETACH_CLIP_CONSTANTS_H
#define FVM_DETACH_CLIP_CONSTANTS_H /* Forced-detachment service guard. */
#define FVM_DETACH_CLIP_INPUT_ROOT 0U /* [GC:] Clip retained during map preparation. */
#define FVM_DETACH_CLIP_ROOT_COUNT 1U /* [GC:] Explicit native clip operand. */
/* [GC:] Dynamic removal retains its explicit clip while checking native ancestors. */
#define FVM_REMOVE_DYNAMIC_CLIP_INPUT_ROOT 0U
#define FVM_REMOVE_DYNAMIC_CLIP_ROOT_COUNT 1U /* [GC:] Single native instance operand. */
#endif
