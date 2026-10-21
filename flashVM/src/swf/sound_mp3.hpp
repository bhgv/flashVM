/* Stack-only scalar MPEG Layer-III decoding with exact contiguous-frame validation. */
#ifndef FVM_SWF_SOUND_MP3_HPP
#define FVM_SWF_SOUND_MP3_HPP /* Guard private native decoder state. */
#include "flashvm/swf_sound.h"
#include "sound_mp3_constants.h"
#define mp3dec_init fvm_minimp3_init /* Match the private static upstream implementation. */
#define mp3dec_decode_frame fvm_minimp3_decode_frame /* Keep host decoder symbols independent. */
#include "../../external/minimp3/upstream/minimp3.h"
#undef mp3dec_init
#undef mp3dec_decode_frame
/* Complete packet geometry copied only after header and payload bounds validate. */
struct FVM_SWFMP3Frame
{
    size_t bytes; /* Exactly one complete MPEG packet, excluding any SWF prefix. */
    unsigned int rate; /* Effective MPEG sample rate, independent of SWF advisory flags. */
    unsigned int channels; /* Effective interleaving, one or two native S16 channel values. */
    unsigned int samples; /* Fixed 576 or 1152 source frames for this packet. */
};
/* Reject skipped bytes/reserved fields; infer free-format from three consecutive matching headers. */
FVM_Result fvm_swf_mp3_probe(const unsigned char *, size_t, FVM_SWFMP3Frame *);
/* Validate complete event MPEG data before decoding the selected latency-trimmed range.
 * Stack-only two-pass extraction; metadata and output storage remain caller-owned. */
FVM_Result fvm_swf_mp3_read(const FVM_SWFSoundInfo &, unsigned long first, size_t frames,
                            short *, size_t capacity_values, size_t *written_values);
/* Native decoder ownership is entirely stack-local; retain between packets of the same stream. */
struct FVM_SWFMP3Decoder
{
    mp3dec_t state; /* Private upstream reservoir/filter state, never a managed/VM reference. */
    mp3d_sample_t pcm[FVM_MP3_MAX_VALUES]; /* Last decoded packet, valid only after decode OK. */
    /* Initialize deterministic state before any source packet can be inspected. */
    FVM_SWFMP3Decoder();
    /* Consume one exact packet. Header errors preserve state; decoder errors are terminal.
     * No allocation, callback or VM. Geometry/output published only on successful decode. */
    FVM_Result decode(const unsigned char *, size_t, FVM_SWFMP3Frame *);
};
#endif
