/* FlashVM: bounded native TextField scalar storage and UTF-16 indexing limits. */
#ifndef FVM_STANDARD_TEXT_CONSTANTS_H
#define FVM_STANDARD_TEXT_CONSTANTS_H /* Native text storage constants guard. */
/* Current shaping/storage envelope; these are implementation bounds, not Flash limits. */
#define FVM_TEXT_FIELD_MAX_BYTES 16384UL
#define FVM_TEXT_FIELD_MAX_SCALARS 4096UL
#define FVM_TEXT_FIELD_MAX_UNITS 8192UL
#define FVM_TEXT_STYLE_NO_KERNING 4UL /* Packed field-style flag; low bits retain legacy alignment. */
#define FVM_TEXT_STYLE_HAS_SPACING 16UL /* Optional signed spacing number follows packed alignment. */
#define FVM_TEXT_STYLE_MAX_SPACING 4096.0 /* Bounded native extra pixels after each supported character. */
#define FVM_TEXT_STYLE_UNDERLINE 32UL /* Packed uniform underline; absent legacy records restore false. */
#define FVM_TEXT_FIELD_GUTTER 4.0 /* Total horizontal/vertical native text padding. */
#define FVM_TEXT_FIELD_MIN_INNER_WIDTH (1.0 / 64.0) /* Progress on oversized graphemes. */
#define FVM_TEXT_FIELD_MAX_EXTENT 1000000.0 /* Maximum admitted outer field dimension. */
#define FVM_TEXT_FIELD_WRITABLE_PROPERTIES 7U /* Text, policies, dimensions, color and autoSize. */
#define FVM_TEXT_FIELD_METHOD_SELECTORS 4U /* Stable format-method selector prefix length. */
#define FVM_TEXT_FIELD_WRITABLE_SELECTORS (FVM_TEXT_FIELD_METHOD_SELECTORS + FVM_TEXT_FIELD_WRITABLE_PROPERTIES)
#define FVM_TEXT_STATE_MULTILINE 1UL /* Legacy low bit of the first field policy byte. */
#define FVM_TEXT_STATE_AUTO_SIZE_LEFT 2UL /* Automatic fitting without adding payload bytes. */
#define FVM_TEXT_STATE_POLICY_MASK (FVM_TEXT_STATE_MULTILINE | FVM_TEXT_STATE_AUTO_SIZE_LEFT)
#define FVM_TEXT_FIELD_GEOMETRY_TOLERANCE (1.0 / 64.0) /* One native font subpixel on restored automatic dimensions. */
#endif
