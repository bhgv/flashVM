/* Windows SAPI event masks. Include after the SDK declares ULONGLONG/SPEI_*. */
#ifndef FVM_SPEECH_WINDOWS_CONSTANTS_H                  /* Keep the private mask declarations unique. */
#define FVM_SPEECH_WINDOWS_CONSTANTS_H                  /* Mark this Windows-only constants header included. */

#define FVM_WINDOWS_SAPI_EVENT_ONE ((ULONGLONG)1)       /* Cast before shifting; Windows long remains 32-bit on x64. */
#define FVM_WINDOWS_SAPI_FINAL_EVENTS                   /* Final recognition plus the SDK's required flag-check bits. */  \
    ((FVM_WINDOWS_SAPI_EVENT_ONE << SPEI_RECOGNITION) | /* Subscribe to completed transcripts, not interim hypotheses. */ \
     (FVM_WINDOWS_SAPI_EVENT_ONE << SPEI_RESERVED1) |   /* Retain the first required SDK flag-check bit. */               \
     (FVM_WINDOWS_SAPI_EVENT_ONE << SPEI_RESERVED2))    /* Retain the second required SDK flag-check bit. */

#endif                                                  /* End the private include guard; no C++11 ull literal is required. */
