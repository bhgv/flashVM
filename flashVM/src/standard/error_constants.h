/* FlashVM: stable callback identities for Error construction, invocation and value access. */
#ifndef FVM_STANDARD_ERROR_CONSTANTS_H
#define FVM_STANDARD_ERROR_CONSTANTS_H
/* Existing Error.toString graph callback identity. */
#define FVM_ERROR_CALLBACK_STRING 301UL
/* Existing Error constructor graph callback identity. */
#define FVM_ERROR_CALLBACK_CONSTRUCTOR 302UL
/* Appended identity for loaded Error() invocation on an existing receiver. */
#define FVM_ERROR_CALLBACK_CALL 303UL
#endif
