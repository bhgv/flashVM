/* FlashVM: root streaming-sound segments, frame blocks and explicit native PCM extraction. */
#ifndef FLASHVM_SWF_STREAM_SOUND_H
#define FLASHVM_SWF_STREAM_SOUND_H /* Guard the C89 streaming sound inspection ABI. */
#include "swf_sound.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Copied SoundStreamHead[2] record; playback geometry is advisory, not a conversion request. */
    typedef struct FVM_SWFStreamSoundInfo
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFStreamSoundInfo). */
        size_t stream_index; /* Zero-based header occurrence; a later header starts a new segment. */
        unsigned int header_tag; /* Wire tag 18 or 45. */
        unsigned int frame; /* One-based root frame containing this header. */
        unsigned int format; /* Raw compression nibble; compressed bytes remain opaque. */
        unsigned int sample_rate; /* Nominal stream rate, using the same integer convention as DefineSound. */
        unsigned int bits_per_sample; /* Stream sample-width flag, 8 or 16. */
        unsigned int channels; /* Stream mono/stereo header geometry. */
        unsigned int playback_sample_rate; /* Advisory nominal playback rate. */
        unsigned int playback_bits_per_sample; /* Advisory playback sample width. */
        unsigned int playback_channels; /* Advisory playback channel count. */
        unsigned int average_samples; /* Advisory average frames per block, never a PCM block length. */
        int latency_seek; /* Signed MP3 latency field; zero for other compression formats. */
    } FVM_SWFStreamSoundInfo;
    /* Borrowed root block bytes with copied frame and PCM geometry. */
    typedef struct FVM_SWFStreamSoundBlock
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFStreamSoundBlock). */
        size_t stream_index; /* Header occurrence governing this block. */
        size_t block_index; /* Zero-based block ordinal within that segment. */
        unsigned int frame; /* One-based root frame; gaps do not synthesize blocks. */
        int sample_count_known; /* Nonzero for supported uncompressed PCM geometry only. */
        size_t sample_count; /* Actual PCM frames inferred from bytes, or zero when unknown. */
        const unsigned char *data; /* Borrowed until successful load_memory or context destruction. */
        size_t data_size; /* Complete block payload, including codec-specific prefixes when opaque. */
    } FVM_SWFStreamSoundBlock;
    /* Count header segments after validating all root stream controls and PCM block alignment. */
    FVM_API FVM_Result fvm_swf_stream_sound_count(FVM_Context *, size_t *);
    /* Copy one header segment; invalid/missing requests leave output unchanged. */
    FVM_API FVM_Result fvm_swf_stream_sound_at(FVM_Context *, size_t, FVM_SWFStreamSoundInfo *);
    /* Count blocks governed by one header, including explicit zero-length PCM blocks. */
    FVM_API FVM_Result fvm_swf_stream_sound_block_count(FVM_Context *, size_t, size_t *);
    /* Copy one segment-relative block view after complete root validation. */
    FVM_API FVM_Result fvm_swf_stream_sound_block_at(FVM_Context *, size_t, size_t, FVM_SWFStreamSoundBlock *);
    /* Decode a bounded PCM range without allocations/callbacks; written counts channel values and is cleared first. */
    FVM_API FVM_Result fvm_swf_stream_sound_read_pcm(FVM_Context *, size_t, size_t, size_t, size_t, short *, size_t, size_t *);
#ifdef __cplusplus
}
#endif
#endif
