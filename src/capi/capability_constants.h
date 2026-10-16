/* FlashVM: fixed-width bearer target identities retained by the shared broker. */
#ifndef FVM_CAPABILITY_CONSTANTS_H
#define FVM_CAPABILITY_CONSTANTS_H /* Shared capability target representation guard. */
#define FVM_CAPABILITY_TOKEN_BYTES 32U /* ASCII hexadecimal representation of a random 128-bit target. */
#define FVM_CAPABILITY_TOKEN_STORAGE 33U /* Fixed target bytes plus terminating NUL for URI formatting. */
#define FVM_CAPABILITY_PEER_SOURCE_BYTES 32U /* Process nonce carried by resource requests. */
#define FVM_CAPABILITY_PEER_CONTEXT_HEX_BYTES 8U /* Hex digits for the wire's unsigned 32-bit context ID. */
#define FVM_CAPABILITY_PEER_KEY_STORAGE (FVM_CAPABILITY_PEER_SOURCE_BYTES + FVM_CAPABILITY_PEER_CONTEXT_HEX_BYTES + 1U) /* Nonce, context and NUL. */
#define FVM_CAPABILITY_PEER_CONTEXT_FORMAT "%08lx" /* Fixed context suffix after validated u32 decoding. */
#endif
