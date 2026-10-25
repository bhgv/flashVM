/* Standalone image signatures, source profiles and pixel geometry; no native-layout dependency. */
#ifndef FVM_IMAGE_LOADING_CONSTANTS_H
#define FVM_IMAGE_LOADING_CONSTANTS_H
#define FVM_IMAGE_PNG_SIGNATURE_BYTES 8U      /* Complete fixed PNG signature. */
#define FVM_IMAGE_GIF_SIGNATURE_BYTES 6U      /* Supported static GIF89a header. */
#define FVM_IMAGE_JPEG_SIGNATURE_BYTES 2U     /* JPEG start-of-image marker. */
#define FVM_IMAGE_SWF_SIGNATURE_BYTES 3U      /* FWS or CWS signature; importer validates the rest. */
#define FVM_IMAGE_JPEG_MARKER 255U            /* JPEG marker prefix byte. */
#define FVM_IMAGE_JPEG_SOI 216U               /* JPEG SOI marker byte. */
#define FVM_IMAGE_JPEG_MIN_SWF 6U             /* Standalone JPEG loading profile. */
#define FVM_IMAGE_LOSSLESS_MIN_SWF 8U         /* PNG/GIF loading profile. */
#define FVM_IMAGE_PROGRESSIVE_JPEG_MIN_SWF 8U /* Progressive standalone JPEG loading profile. */
#define FVM_IMAGE_CONTENT_MAX_DIMENSION 2880U /* Imported codec dimension bound in metadata. */
#define FVM_IMAGE_RGBA_CHANNELS 4U            /* Straight red/green/blue/alpha bytes per pixel. */
#endif
