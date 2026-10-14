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
/* Leading-bit marker identifying a continuation byte. */
#define FVM_UTF8_CONTINUATION_MARKER 0x80U
/* Smallest non-overlong two-byte scalar value. */
#define FVM_UTF8_TWO_BYTE_SCALAR_MIN 0x80UL
#endif
