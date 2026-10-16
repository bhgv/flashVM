/* FlashVM: radix bounds and legacy parseInt prefix rules, distinct from host inference. */
#ifndef FVM_STANDARD_PARSING_CONSTANTS_H
#define FVM_STANDARD_PARSING_CONSTANTS_H /* Guard the private parsing constants. */
#define FVM_PARSING_RADIX_MIN 2U /* First accepted explicit base. */
#define FVM_PARSING_RADIX_MAX 36U /* Last accepted explicit base. */
#define FVM_PARSING_RADIX_OCTAL 8U /* Only complete raw octal strings infer this base. */
#define FVM_PARSING_RADIX_DECIMAL 10U /* Default when raw octal/hex detection fails. */
#define FVM_PARSING_RADIX_HEX 16U /* Default for an unsigned raw hexadecimal prefix. */
#define FVM_PARSING_HEX_PREFIX_BYTES 2U /* ASCII zero followed by x or X. */
#define FVM_PARSING_SIGNED_HEX_MIN_RADIX 34U /* First base accepting x as a digit. */
#define FVM_PARSING_LETTER_DIGIT_FIRST 10 /* Numeric value of a/A. */
#define FVM_PARSING_INT_SPACE_TAB 9U /* AVM1 parseInt strips these four ASCII units only. */
#define FVM_PARSING_INT_SPACE_LF 10U /* Line feed. */
#define FVM_PARSING_INT_SPACE_CR 13U /* Carriage return. */
#define FVM_PARSING_INT_SPACE_ASCII 32U /* Ordinary space. */
#endif
