/* FlashVM: Unicode scalar UTF-8 bounds and byte-field masks. */
#ifndef FVM_UNICODE_CONSTANTS_H
#define FVM_UNICODE_CONSTANTS_H /* Encoding constants guard. */
/* First byte outside the ASCII range. */
#define FVM_UTF8_ASCII_LIMIT 0x80U
/* First valid two-byte lead; excludes overlong encodings. */
#define FVM_UTF8_TWO_BYTE_LEAD_MIN 0xc2U
/* Last two-byte lead. */
#define FVM_UTF8_TWO_BYTE_LEAD_MAX 0xdfU
/* First three-byte lead. */
#define FVM_UTF8_THREE_BYTE_LEAD_MIN 0xe0U
/* Last three-byte lead. */
#define FVM_UTF8_THREE_BYTE_LEAD_MAX 0xefU
/* First four-byte lead. */
#define FVM_UTF8_FOUR_BYTE_LEAD_MIN 0xf0U
/* Last Unicode four-byte lead. */
#define FVM_UTF8_FOUR_BYTE_LEAD_MAX 0xf4U
/* Five payload bits in a two-byte lead. */
#define FVM_UTF8_TWO_BYTE_PAYLOAD_MASK 31U
/* Four payload bits in a three-byte lead. */
#define FVM_UTF8_THREE_BYTE_PAYLOAD_MASK 15U
/* Three payload bits in a four-byte lead. */
#define FVM_UTF8_FOUR_BYTE_PAYLOAD_MASK 7U
/* Smallest non-overlong three-byte scalar. */
#define FVM_UTF8_THREE_BYTE_SCALAR_MIN 0x800UL
/* Smallest non-overlong four-byte scalar. */
#define FVM_UTF8_FOUR_BYTE_SCALAR_MIN 0x10000UL
/* Select the leading two bits of a continuation byte. */
#define FVM_UTF8_CONTINUATION_CLASS_MASK 0xc0U
/* Six payload bits in a continuation byte. */
#define FVM_UTF8_CONTINUATION_PAYLOAD_MASK 63U
/* Bits contributed by each continuation byte. */
#define FVM_UTF8_CONTINUATION_PAYLOAD_BITS 6U
/* Last Unicode code point. */
#define FVM_UNICODE_SCALAR_MAX 0x10ffffUL
/* First surrogate code unit; invalid in scalar UTF-8. */
#define FVM_UNICODE_SURROGATE_MIN 0xd800UL
/* Last surrogate code unit; invalid in scalar UTF-8. */
#define FVM_UNICODE_SURROGATE_MAX 0xdfffUL
/* Replacement code unit used by legacy character operations for surrogate inputs. */
#define FVM_UNICODE_REPLACEMENT_CHARACTER 0xfffdU
/* Largest byte belonging to a UTF-8 continuation field. */
#define FVM_UTF8_CONTINUATION_BYTE_MAX 0xbfU
/* Maximum storage required for one scalar UTF-8 sequence. */
#define FVM_UTF8_MAX_SEQUENCE_BYTES 4U
/* Scalar offset subtracted before splitting a supplementary value into UTF-16 units. */
#define FVM_UTF16_SUPPLEMENTARY_OFFSET 0x10000UL
/* Leading UTF-16 surrogate base; add the high supplementary payload bits. */
#define FVM_UTF16_HIGH_SURROGATE_BASE 0xd800U
/* Last leading UTF-16 surrogate code unit, for pair recognition. */
#define FVM_UTF16_HIGH_SURROGATE_MAX 0xdbffU
/* Last trailing UTF-16 surrogate code unit, for pair recognition. */
#define FVM_UTF16_LOW_SURROGATE_MAX 0xdfffU
/* Trailing UTF-16 surrogate base; add the low supplementary payload bits. */
#define FVM_UTF16_LOW_SURROGATE_BASE 0xdc00U
/* Number of supplementary payload bits carried by either surrogate unit. */
#define FVM_UTF16_PAIR_PAYLOAD_BITS 10U
/* Mask selecting the low payload for a trailing surrogate. */
#define FVM_UTF16_PAIR_PAYLOAD_MASK 0x3ffUL
/* WTF-8 accepts this otherwise scalar-invalid lead when encoding lone surrogates. */
#define FVM_WTF8_SURROGATE_LEAD 0xedU
/* First second-byte value selecting the surrogate range under the ED lead. */
#define FVM_WTF8_SURROGATE_SECOND_MIN 0xa0U
/* A lone surrogate is represented by a complete three-byte WTF-8 sequence. */
#define FVM_WTF8_SURROGATE_SEQUENCE_BYTES 3U
/* Decoded contribution of the ED lead before adding the two continuation payloads. */
#define FVM_WTF8_SURROGATE_DECODE_BASE 0xd000UL
/* Leading-bit marker identifying a continuation byte. */
#define FVM_UTF8_CONTINUATION_MARKER 0x80U
/* Smallest non-overlong two-byte scalar value. */
#define FVM_UTF8_TWO_BYTE_SCALAR_MIN 0x80UL
/* Double-exact modulus for wrapping String.fromCharCode arguments to one UTF-16 unit. */
#define FVM_UTF16_UNIT_MODULUS 65536.0
/* Capital Greek sigma requiring contextual lowercase handling in Unicode host casing. */
#define FVM_UNICODE_CAPITAL_SIGMA 0x3a3UL
/* Lowercase final sigma selected when no following cased scalar is present. */
#define FVM_UNICODE_FINAL_SIGMA 0x3c2UL
#endif
