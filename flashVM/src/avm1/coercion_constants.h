/* FlashVM: legacy numeric syntax, selected method names and supported whitespace code points. */
#ifndef FVM_AVM1_COERCION_CONSTANTS_H
#define FVM_AVM1_COERCION_CONSTANTS_H    /* Coercion syntax guard. */
#define FVM_COERCION_RADIX_DECIMAL 10U   /* Default decimal digit accumulation. */
#define FVM_COERCION_RADIX_OCTAL 8U      /* Leading-zero integer syntax from SWF6 onward. */
#define FVM_COERCION_RADIX_HEX 16U       /* Raw 0x-prefixed integer syntax from SWF6 onward. */
#define FVM_COERCION_HEX_LETTER_FIRST 10 /* Numeric value represented by ASCII a/A. */
#define FVM_COERCION_INVALID_DIGIT (-1)  /* Sentinel for a byte outside supported numeric digits.  \
                                          */
#define FVM_COERCION_HEX_PREFIX_BYTES                                                              \
    2U /* Historical raw offset removed for hexadecimal parsing. */
#define FVM_COERCION_EXPONENT_SUFFIX_BYTES                                                         \
    32U /* Buffer for e, signed Int32 exponent and terminator. */
#define FVM_COERCION_VALUE_OF_NAME "valueOf" /* Method selected for ordinary primitive coercion.   \
                                              */
#define FVM_COERCION_VALUE_OF_BYTES                                                                \
    (sizeof(FVM_COERCION_VALUE_OF_NAME) - 1U) /* Count excludes NUL. */
#define FVM_COERCION_TO_STRING_NAME                                                                \
    "toString" /* String coercion and Date's default primitive hint. */
#define FVM_COERCION_TO_STRING_BYTES (sizeof(FVM_COERCION_TO_STRING_NAME) - 1U) /* Excludes NUL.   \
                                                                                 */
#define FVM_COERCION_SPACE_TAB 0x0009UL         /* Horizontal tab accepted before decimal input. */
#define FVM_COERCION_SPACE_LF 0x000aUL          /* Line feed accepted before decimal input. */
#define FVM_COERCION_SPACE_CR 0x000dUL          /* Carriage return accepted before decimal input. */
#define FVM_COERCION_SPACE_ASCII 0x0020UL       /* Ordinary ASCII space. */
#endif
