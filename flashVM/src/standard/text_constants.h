/* FlashVM: bounded native TextField scalar storage and UTF-16 indexing limits. */
#ifndef FVM_STANDARD_TEXT_CONSTANTS_H
#define FVM_STANDARD_TEXT_CONSTANTS_H /* Native text storage constants guard. */
/* Current shaping/storage envelope; these are implementation bounds, not Flash limits. */
#define FVM_TEXT_FONT_MAX_NAME 255U /* Maximum registered family-name bytes, excluding terminator. */
#define FVM_TEXT_FONT_KEY_PREFIX 2U /* Leading NUL plus nonzero variant isolates private slots. */
#define FVM_TEXT_FONT_MAX_FACES 16U /* Aggregate regular/styled registrations per context. */
#define FVM_TEXT_FONT_MAX_BYTES (64UL * 1024UL * 1024UL) /* Aggregate owned font-source budget. */
#define FVM_TEXT_FIELD_MAX_BYTES 16384UL
#define FVM_TEXT_FIELD_MAX_SCALARS 4096UL
#define FVM_TEXT_FIELD_MAX_UNITS 8192UL
#define FVM_TEXT_STYLE_NO_KERNING 4UL /* Packed field-style flag; low bits retain legacy alignment. */
#define FVM_TEXT_STYLE_HAS_SPACING 16UL /* Optional signed spacing number follows packed alignment. */
#define FVM_TEXT_STYLE_MAX_SPACING 4096.0 /* Bounded native extra pixels after each supported character. */
#define FVM_TEXT_STYLE_UNDERLINE 32UL /* Packed uniform underline; absent legacy records restore false. */
#define FVM_TEXT_STYLE_BOLD 64UL /* Select the explicit bold family face in snapshots. */
#define FVM_TEXT_STYLE_ITALIC 128UL /* Select the explicit italic family face in snapshots. */
#define FVM_TEXT_STYLE_JUSTIFY 256UL /* Justified paragraph policy, distinct from low alignment/kerning bits. */
#define FVM_TEXT_JUSTIFY_MIN_VERSION 8U /* AS2 justified paragraphs became available in SWF8. */
#define FVM_TEXT_STYLE_FLAGS (FVM_TEXT_STYLE_NO_KERNING | FVM_TEXT_STYLE_HAS_SPACING | FVM_TEXT_STYLE_UNDERLINE | FVM_TEXT_STYLE_BOLD | FVM_TEXT_STYLE_ITALIC | FVM_TEXT_STYLE_JUSTIFY) /* Packed non-alignment bits. */
#define FVM_TEXT_FIELD_GUTTER 4.0 /* Total horizontal/vertical native text padding. */
#define FVM_TEXT_FIELD_MIN_INNER_WIDTH (1.0 / 64.0) /* Progress on oversized graphemes. */
#define FVM_TEXT_FIELD_MAX_EXTENT 1000000.0 /* Maximum admitted outer field dimension. */
#define FVM_TEXT_FIELD_WRITABLE_PROPERTIES 9U /* Text, policies, dimensions, color, autoSize and position. */
#define FVM_TEXT_FIELD_METHOD_SELECTORS 4U /* Stable format-method selector prefix length. */
#define FVM_TEXT_FIELD_WRITABLE_SELECTORS (FVM_TEXT_FIELD_METHOD_SELECTORS + FVM_TEXT_FIELD_WRITABLE_PROPERTIES)
#define FVM_TEXT_STATE_MULTILINE 1UL /* Legacy low bit of the first field policy byte. */
#define FVM_TEXT_STATE_AUTO_SIZE_LEFT 2UL /* Automatic fitting without adding payload bytes. */
#define FVM_TEXT_STATE_HAS_POSITION 4UL /* Optional x/y numbers follow policy bytes. */
#define FVM_TEXT_STATE_CENTER 8UL /* Center automatic anchor, requires fitting bit. */
#define FVM_TEXT_STATE_RIGHT 16UL /* Right automatic anchor, exclusive with center. */
#define FVM_TEXT_STATE_POLICY_MASK (FVM_TEXT_STATE_MULTILINE | FVM_TEXT_STATE_AUTO_SIZE_LEFT | FVM_TEXT_STATE_HAS_POSITION | FVM_TEXT_STATE_CENTER | FVM_TEXT_STATE_RIGHT)
#define FVM_TEXT_ANCHOR_LEFT 0UL /* Preserve left edge while fitting. */
#define FVM_TEXT_ANCHOR_CENTER 1UL /* Preserve horizontal center while fitting. */
#define FVM_TEXT_ANCHOR_RIGHT 2UL /* Preserve right edge while fitting. */
#define FVM_TEXT_FIELD_MAX_POSITION 1000000.0 /* Signed parent-space translation envelope. */
#define FVM_TEXT_FIELD_GEOMETRY_TOLERANCE (1.0 / 64.0) /* One native font subpixel on restored automatic dimensions. */
#endif
