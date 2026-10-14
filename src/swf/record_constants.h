/* FlashVM: SWF movie-prefix and tag-record framing shared with action traversal. */
#ifndef FVM_SWF_RECORD_CONSTANTS_H
#define FVM_SWF_RECORD_CONSTANTS_H /* Record-layout guard. */
#include "format_constants.h"
#define FVM_SWF_PREFIX_BYTES 8U     /* Signature, version and declared uncompressed length. */
#define FVM_SWF_BYTE_BITS 8U        /* Byte width used to round packed RECT bits. */
#define FVM_SWF_RECT_WIDTH_BITS 5U  /* Leading RECT field containing each coordinate's bit width.  \
                                     */
#define FVM_SWF_RECT_COORDINATES 4U /* Xmin, Xmax, Ymin and Ymax coordinates in a RECT. */
#define FVM_SWF_RECT_WIDTH_SHIFT                                                                   \
    (FVM_SWF_BYTE_BITS - FVM_SWF_RECT_WIDTH_BITS) /* Width is in the first byte's high bits. */
#define FVM_SWF_FRAME_FIELDS_BYTES 4U    /* UI16 fixed-point frame rate and UI16 frame count. */
#define FVM_SWF_TAG_HEADER_BYTES 2U      /* UI16 combining tag ID and short payload length. */
#define FVM_SWF_LONG_LENGTH_BYTES 4U     /* UI32 payload length after the short-length sentinel. */
#define FVM_SWF_LONG_LENGTH_SENTINEL 63U /* All six short-length bits set selects a UI32 length.   \
                                          */
#endif
