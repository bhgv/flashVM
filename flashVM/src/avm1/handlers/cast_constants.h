/* FlashVM: cast operands and the discarded primitive boxing result. */
#ifndef FVM_CAST_CONSTANTS_H
#define FVM_CAST_CONSTANTS_H   /* Cast service root-layout guard. */
#define FVM_CAST_VALUE_ROOT 0U /* [GC:] Original value returned unchanged on a match. */
#define FVM_CAST_CLASS_ROOT 1U /* [GC:] Original class value retained through boxing callbacks. */
#define FVM_CAST_BOX_ROOT 2U   /* [GC:] Discarded primitive wrapper, created for its side effects. \
                                */
#define FVM_CAST_ROOT_COUNT 3U /* [GC:] Original operands and temporary wrapper. */
#endif
