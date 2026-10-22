/* Private OpenType shaping feature protocol values; public selection flags live in font.h. */
#ifndef FVM_FONT_SHAPE_CONSTANTS_H
#define FVM_FONT_SHAPE_CONSTANTS_H /* Scope pinned HarfBuzz feature descriptors. */
#define FVM_FONT_KERN_TAG 0x6B65726EUL /* Big-endian OpenType 'kern' feature identity. */
#define FVM_FONT_LIGA_TAG 0x6C696761UL /* OpenType standard optional ligatures. */
#define FVM_FONT_CLIG_TAG 0x636C6967UL /* OpenType contextual optional ligatures. */
#define FVM_FONT_FEATURE_CAPACITY 3U /* kern plus liga/clig explicit overrides. */
#define FVM_FONT_FEATURE_BEGIN 0U /* Apply explicit feature selection from the first cluster. */
#define FVM_FONT_FEATURE_END ((unsigned int)-1) /* HarfBuzz whole-buffer terminal cluster sentinel. */
#define FVM_FONT_BIDI_MAX_SCALARS 1000000UL /* Maximum UTF-32 paragraph length accepted by the C bridge. */
#define FVM_FONT_UNICODE_MAX 0x10FFFFU /* Largest Unicode scalar representation. */
#define FVM_FONT_SURROGATE_BEGIN 0xD800U /* First non-scalar UTF-16 surrogate code point. */
#define FVM_FONT_SURROGATE_END 0xDFFFU /* Last non-scalar UTF-16 surrogate code point. */
#define FVM_FONT_UNDERLINE_MAX_EXTENT 1000000.0 /* Absolute pixel bound for scalable underline metrics. */
#endif
