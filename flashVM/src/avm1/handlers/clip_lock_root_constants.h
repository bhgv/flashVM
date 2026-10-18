/* FlashVM: native root-lock assignment roots, independent of Action encoding. */
#ifndef FVM_CLIP_LOCK_ROOT_CONSTANTS_H
#define FVM_CLIP_LOCK_ROOT_CONSTANTS_H   /* Root-lock service guard. */
#define FVM_CLIP_LOCK_ROOT_CLIP_ROOT 0U  /* [GC:] Native instance retained during preparation. */
#define FVM_CLIP_LOCK_ROOT_INPUT_ROOT 1U /* [GC:] Original assigned value. */
#define FVM_CLIP_LOCK_ROOT_VALUE_ROOT 2U /* [GC:] Converted Boolean flag. */
#define FVM_CLIP_LOCK_ROOT_ROOT_COUNT 3U /* [GC:] Complete root-lock assignment layout. */
/* Boolean mode selectors used by the native accessor bridges. */
#define FVM_CLIP_LOCK_ROOT_READ 0
#define FVM_CLIP_LOCK_ROOT_WRITE 1
#endif
