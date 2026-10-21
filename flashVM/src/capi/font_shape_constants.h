/* Private OpenType shaping feature protocol values; public selection flags live in font.h. */
#ifndef FVM_FONT_SHAPE_CONSTANTS_H
#define FVM_FONT_SHAPE_CONSTANTS_H /* Scope pinned HarfBuzz feature descriptors. */
#define FVM_FONT_KERN_TAG 0x6B65726EUL /* Big-endian OpenType 'kern' feature identity. */
#define FVM_FONT_FEATURE_BEGIN 0U /* Apply explicit feature selection from the first cluster. */
#define FVM_FONT_FEATURE_END ((unsigned int)-1) /* HarfBuzz whole-buffer terminal cluster sentinel. */
#endif
