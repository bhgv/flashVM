/* OS: all supported targets. Reject invalid media geometry before output callbacks. */
#include "session.hpp" /* Share the public frame layout. */
#include <limits.h> /* Obtain the native short representation limits. */
/* Validate a borrowed view using overflow-safe size arithmetic. */
bool fvm_media_frame_valid(const FVM_MediaFrame *frame, int kind) /* Inspect metadata only. */
{ /* Begin portable buffer validation. */
    if (!frame || frame->kind != kind || !frame->data || !frame->size) /* Require a populated buffer of the expected kind. */
        return false; /* Require a populated buffer of the expected kind. */
    if (kind == FVM_MEDIA_AUDIO) /* Validate interleaved signed-16 PCM. */
    { /* Keep channel multiplication inside size_t bounds. */
        if (sizeof(short) != 2 || SHRT_MAX != 32767 || !frame->sample_rate || !frame->channels || !frame->samples) /* Require the documented PCM representation. */
            return false; /* Require the documented PCM representation. */
        size_t values = frame->size / sizeof(short); /* Convert accessible bytes into channel-value capacity. */
        return frame->samples <= values / frame->channels && /* Return the result produced by the following backend operation. */
               frame->size % sizeof(short) == 0; /* Verify every declared sample fits. */
    } /* Finish audio validation. */
    if (kind != FVM_MEDIA_VIDEO || !frame->width || !frame->height) /* Reject unknown kinds and empty images. */
        return false; /* Reject unknown kinds and empty images. */
    if (frame->width > frame->stride / FVM_MEDIA_RGBA_BYTES_PER_PIXEL || frame->height > frame->size / frame->stride) /* Check RGBA rows without overflowing products. */
        return false; /* Check RGBA rows without overflowing products. */
    return true; /* The accessible buffer contains the complete frame. */
} /* Finish metadata validation. */
