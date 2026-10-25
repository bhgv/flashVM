/* FlashVM: AVM1 URL codec bounds and stable global-function callback range. */
#ifndef FVM_STANDARD_URL_CONSTANTS_H
#define FVM_STANDARD_URL_CONSTANTS_H           /* Guard URL encoding and registry constants. */
#define FVM_URL_MIN_SWF_VERSION 5U             /* escape/unescape first appeared in Flash Player 5. */
#define FVM_URL_ESCAPE_BYTES 3U                /* Percent marker plus exactly two hexadecimal digits. */
#define FVM_URL_MAX_EXPANSION 3U               /* Each encoded input byte produces at most three ASCII bytes. */
#define FVM_URL_HEX_RADIX 16U                  /* Numeric base of one encoded byte. */
#define FVM_URL_HEX_ALPHA_FIRST 10             /* Numeric value of hexadecimal a/A. */
#define FVM_NUMERIC_GLOBAL_CALLBACK_BASE 150UL /* Existing numeric-global graph IDs; append only. */
#define FVM_NUMERIC_LEGACY_GLOBAL_COUNT 7U     /* Existing global callbacks occupy IDs 150 through 156. */
#define FVM_URL_ESCAPE_CALLBACK 161UL          /* First free numeric ID after existing constants 158--160. */
#define FVM_URL_UNESCAPE_CALLBACK 162UL        /* Stable decoder ID, distinct from all existing callbacks. */
#endif
