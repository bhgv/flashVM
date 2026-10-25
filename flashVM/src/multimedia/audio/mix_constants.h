/* OS: portable native PCM. Scoped representation bounds for exact summed S16 mixing. */
#ifndef FVM_AUDIO_MIX_CONSTANTS_H       /* Guard these operation-specific numeric bounds. */
#define FVM_AUDIO_MIX_CONSTANTS_H       /* Keep public protocol IDs separate from native arithmetic. */
#define FVM_AUDIO_MIX_BYTE_BITS 8U      /* Native S16 byte geometry requires octet storage. */
#define FVM_AUDIO_MIX_S16_BYTES 2U      /* Each input/output channel value occupies two octets. */
#define FVM_AUDIO_MIX_S16_MIN (-32768L) /* Minimum saturated signed-16 output value. */
#define FVM_AUDIO_MIX_S16_MAX 32767L    /* Maximum saturated signed-16 output value. */
#endif                                  /* Finish scoped mixer representation bounds. */
