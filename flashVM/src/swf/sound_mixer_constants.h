/* FlashVM: independent meanings for the native event mixer scratch geometry. */
#ifndef FVM_SWF_SOUND_MIXER_CONSTANTS_H
#define FVM_SWF_SOUND_MIXER_CONSTANTS_H /* Guard private mixer bounds. */
#define FVM_SWF_SOUND_MIXER_CHUNK 256U  /* Frames in one bounded allocation-free mixing pass. */
#define FVM_SWF_SOUND_MIXER_CHANNELS 2U /* Maximum fixed interleaved channel count. */
#define FVM_SWF_SOUND_RELEASE_DIVISOR 200U /* Five milliseconds in the configured output clock. */
#define FVM_SWF_SOUND_RELEASE_MIN 2U /* A release needs distinct full-gain and zero-gain endpoints. */
#define FVM_SWF_SOUND_MIXER_HORIZON_GUARD 3U /* Two buffered source frames plus one rational rounding guard. */
#endif
