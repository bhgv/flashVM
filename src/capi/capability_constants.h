/* FlashVM: fixed-width bearer target identities retained by the shared broker. */
#ifndef FVM_CAPABILITY_CONSTANTS_H
#define FVM_CAPABILITY_CONSTANTS_H /* Shared capability target representation guard. */
#define FVM_CAPABILITY_TOKEN_BYTES 32U /* ASCII hexadecimal representation of a random 128-bit target. */
#define FVM_CAPABILITY_TOKEN_STORAGE 33U /* Fixed target bytes plus terminating NUL for URI formatting. */
#endif
