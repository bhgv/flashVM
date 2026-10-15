/* FlashVM: String operation bounds and stable native callback transport identities. */
#ifndef FVM_STANDARD_STRING_CONSTANTS_H
#define FVM_STANDARD_STRING_CONSTANTS_H /* Builtin String constants guard. */
/* Retained unloaded-host bound; loaded split uses AS limits and shared execution work. */
#define FVM_STRING_HOST_SPLIT_MAX_FIELDS 65536UL
/* Persisted native callbacks: retain these values for serialized graphs. */
#define FVM_STRING_CALLBACK_LENGTH 501UL         /* String instance length accessor. */
#define FVM_STRING_CALLBACK_FROM_CHAR_CODE 502UL /* Static UTF-16 unit constructor. */
#define FVM_STRING_CALLBACK_CONSTRUCTOR 503UL    /* String class call/construct callback. */
#define FVM_STRING_CALLBACK_METHOD_BASE 510UL    /* Stable offset in the existing method table. */
#endif
