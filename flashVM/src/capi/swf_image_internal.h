/* FlashVM: C89 boundaries for bounded PNG and single-frame GIF89a SWF resources. */
#ifndef FVM_SWF_IMAGE_INTERNAL_H
#define FVM_SWF_IMAGE_INTERNAL_H /* Include guard for embedded lossless image bridges. */
#include "flashvm/flashvm.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* Decode a complete PNG stream into malloc-owned straight RGBA; failed outputs remain empty. */
FVM_Result fvm_swf_decode_png(const unsigned char *, size_t, size_t, unsigned *, unsigned *, unsigned char **);


/* Decode one full-canvas GIF89a frame into malloc-owned RGBA; animations remain unsupported. */
FVM_Result fvm_swf_decode_gif(const unsigned char *, size_t, size_t, unsigned *, unsigned *, unsigned char **);
#ifdef __cplusplus
}
#endif
#endif
