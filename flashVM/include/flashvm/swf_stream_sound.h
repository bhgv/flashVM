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
        const unsigned char *data; /* Live view: until successful reload/context destruction; snapshot view: until snapshot_close. */
        size_t data_size; /* Complete block payload, including codec-specific prefixes when opaque. */
    } FVM_SWFStreamSoundBlock;
    /* Copied MP3STREAMSOUNDDATA prefix plus borrowed MPEG bytes; no decoder is run. */
    typedef struct FVM_SWFStreamSoundMP3Block
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFStreamSoundMP3Block). */
        size_t stream_index; /* Header occurrence governing this block. */
        size_t block_index; /* Encoded block ordinal within that segment. */
        unsigned int frame; /* One-based root movie frame, independent of MPEG frames. */
        unsigned int declared_samples; /* UI16 sample frames/pairs from the wire, not a verified decoded length. */
        int seek_samples; /* Signed SI16 block seek adjustment; header latency is kept separate. */
        const unsigned char *data; /* MPEG span after the four-byte prefix; follows the source view lifetime. */
        size_t data_size; /* MPEG bytes only; zero is permitted without claiming playable audio. */
    } FVM_SWFStreamSoundMP3Block;
    /* Inspect the selected live MP3 prefix; no allocation/callback/decoding; preserve output on failure. */
    FVM_API FVM_Result fvm_swf_stream_sound_mp3_block_at(FVM_Context *, size_t, size_t, FVM_SWFStreamSoundMP3Block *);
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
    /* Independent immutable native encoded segment, explicitly prepared and released by the host. */
    typedef struct FVM_SWFStreamSoundSnapshot FVM_SWFStreamSoundSnapshot;
    /* Copy and index one encoded segment; clear *out first, subject to the runtime memory budget. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_open(FVM_Context *, size_t, FVM_SWFStreamSoundSnapshot **);
    /* Copy the original source/advisory header, preserving output on error. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_info(const FVM_SWFStreamSoundSnapshot *, FVM_SWFStreamSoundInfo *);
    /* Query the prepared block count without rescanning the movie. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_block_count(const FVM_SWFStreamSoundSnapshot *, size_t *);
    /* Borrow an indexed original-wire payload until snapshot_close. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_block_at(const FVM_SWFStreamSoundSnapshot *, size_t, FVM_SWFStreamSoundBlock *);
    /* Inspect the indexed MP3 prefix after source VM teardown; the MPEG span lasts until close. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_mp3_block_at(const FVM_SWFStreamSoundSnapshot *, size_t, FVM_SWFStreamSoundMP3Block *);
    /* Decode a requested range to native S16; written counts channel values and is cleared first. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_read_pcm(const FVM_SWFStreamSoundSnapshot *, size_t, size_t, size_t, short *, size_t, size_t *);
    /* Free the independent snapshot; null is a no-op. */
    FVM_API void fvm_swf_stream_sound_snapshot_close(FVM_SWFStreamSoundSnapshot *);
#ifdef __cplusplus
}
#endif
#endif
