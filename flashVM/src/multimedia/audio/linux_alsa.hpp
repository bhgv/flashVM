/* OS: Linux ALSA, explicitly enabled with a matching static SDK. */
#ifndef FVM_LINUX_ALSA_OUTPUT_HPP     /* Guard the native device state and callback declarations. */
#define FVM_LINUX_ALSA_OUTPUT_HPP     /* Declare the private output interface once. */
#include "../common/session.hpp"      /* Import frame bounds and the C89 bridge ABI. */
#if defined(FVM_HAS_LINUX_ALSA_AUDIO) /* Include the real SDK only in an explicitly enabled build. */
#include <alsa/asoundlib.h>           /* Call the actual ALSA PCM playback API. */

struct FVM_AlsaOutput                 /* Own a native device independently of the VM channel registry. */
{                                     /* Begin Linux device state. */
    snd_pcm_t *pcm;                   /* Own the explicitly opened PCM handle. */
    unsigned int rate;                /* Require this exact stream sample rate. */
    unsigned int channels;            /* Require this exact interleaved channel count. */
    bool prepared;                    /* Reprepare after drop before accepting another frame. */
}; /* Finish Linux device state. */


/* Submit the complete frame without waiting for device buffer space; discard pending output on failure. */
FVM_Result fvm_alsa_output_write(void *, const FVM_MediaFrame *); /* OS: Linux/static ALSA callback. */


/* Drop queued samples immediately, retaining the device for a subsequent write. */
FVM_Result fvm_alsa_output_stop(void *); /* OS: Linux/static ALSA callback. */
#endif                                   /* Finish native-only declarations. */
#endif                                   /* Finish the private header guard. */
