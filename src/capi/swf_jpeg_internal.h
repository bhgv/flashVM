/* FlashVM: C89 boundary for bounded JPEG decoding without C++ longjmp hazards. */
#ifndef FVM_SWF_JPEG_INTERNAL_H
#define FVM_SWF_JPEG_INTERNAL_H /* Include guard for the internal JPEG bridge. */
#include "flashvm/flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Decode complete RGB/grayscale JPEG into malloc-owned straight RGBA; caller frees successful
     * pixels. Failed decoding clears all outputs and never exposes a partial image. */
    FVM_Result fvm_swf_decode_jpeg(const unsigned char *, size_t, size_t, unsigned *, unsigned *,
                                   unsigned char **);
#ifdef __cplusplus
}
#endif
#endif
