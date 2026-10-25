/* FlashVM: portable periodic gradient sampling constants. */
#ifndef FVM_RENDER_GRADIENT_CONSTANTS_H
#define FVM_RENDER_GRADIENT_CONSTANTS_H          /* Guard the private gradient period. */
#define FVM_GRADIENT_REFLECT_PERIOD 2.0          /* One forward and one reverse unit interval. */
#define FVM_GRADIENT_SRGB_DECODE_BREAK 0.04045   /* Encoded sRGB transfer breakpoint. */
#define FVM_GRADIENT_SRGB_ENCODE_BREAK 0.0031308 /* Linear-light sRGB transfer breakpoint. */
#define FVM_GRADIENT_SRGB_SLOPE 12.92            /* Linear segment gain of the encoding transfer. */
#define FVM_GRADIENT_SRGB_OFFSET 0.055           /* Nonlinear encoded-domain offset. */
#define FVM_GRADIENT_SRGB_SCALE 1.055            /* Nonlinear encoded-domain scale. */
#define FVM_GRADIENT_SRGB_EXPONENT 2.4           /* Decoding power; encoding uses its reciprocal. */
#define FVM_GRADIENT_COLOR_BYTE_MAX 255.0        /* Normalize borrowed byte stops to unit channels. */
#endif
