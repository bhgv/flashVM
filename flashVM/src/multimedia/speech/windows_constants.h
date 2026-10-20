/* Windows SAPI event masks. Include after the SDK declares ULONGLONG/SPEI_*. */
#ifndef FVM_SPEECH_WINDOWS_CONSTANTS_H
#define FVM_SPEECH_WINDOWS_CONSTANTS_H

/* Convert before shifting: Windows long is 32-bit even on x64. No C++11 ull
 * literal is required at the first-party C++98 call site. */
#define FVM_WINDOWS_SAPI_EVENT_ONE ((ULONGLONG)1)

/* Same three bits as SPFEI(SPEI_RECOGNITION): final recognition plus both SDK
 * reserved flag-check bits. This does not subscribe to interim transcripts. */
#define FVM_WINDOWS_SAPI_FINAL_EVENTS                                                              \
    ((FVM_WINDOWS_SAPI_EVENT_ONE << SPEI_RECOGNITION) |                                            \
     (FVM_WINDOWS_SAPI_EVENT_ONE << SPEI_RESERVED1) |                                              \
     (FVM_WINDOWS_SAPI_EVENT_ONE << SPEI_RESERVED2))

#endif
