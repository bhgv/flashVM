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
#endif
