/* Explicit native PCM rate/channel conversion over an immutable prepared SWF segment. */
#ifndef FLASHVM_SWF_STREAM_CONVERT_H
#define FLASHVM_SWF_STREAM_CONVERT_H /* Guard the C89 bounded conversion ABI. */
#include "swf_stream_pcm.h"
#define FVM_STREAM_CONVERT_MIN_RATE 5512UL /* Smallest supported output sample clock. */
#define FVM_STREAM_CONVERT_MAX_RATE                                                                \
    192000UL /* Bounds rational phase and interpolation arithmetic. */
#define FVM_STREAM_CONVERT_MAX_FRAMES 1024U /* Maximum output work per successful next call. */
#define FVM_STREAM_CONVERT_BEGIN 1U /* First returned chunk/marker for this encoded block. */
#define FVM_STREAM_CONVERT_END 2U   /* No further output sample positions belong to this block. */
#define FVM_STREAM_CONVERT_EMPTY 4U /* The encoded block itself contains no source samples. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Owned description of converted output; no native pointer or device time is published. */
    typedef struct FVM_SWFStreamConvertedChunk
    {
        size_t struct_size;               /* Caller sets exact sizeof before next. */
        size_t stream_index, block_index; /* Immutable segment and encoded block identities. */
        unsigned int frame; /* Source movie frame; gaps are not synthesized or scheduled. */
        size_t
            first_source; /* First source sample position, clipped to block end for zero markers. */
        unsigned long
            phase;     /* Fractional source position numerator with output-rate denominator. */
        size_t frames; /* Produced output sample frames, not scalar channel values. */
        unsigned long sample_rate; /* Explicit selected output clock. */
        unsigned int channels;     /* Explicit selected output mono/stereo interleaving. */
        unsigned int flags; /* BEGIN/END/EMPTY; a skipped nonempty block has END but not EMPTY. */
    } FVM_SWFStreamConvertedChunk;
    /* Single-owner caller state; snapshot is the sole borrowed native resource. */
    typedef struct FVM_SWFStreamConverter
    {
        size_t struct_size; /* Exact native ABI initialized atomically. */
        const FVM_SWFStreamSoundSnapshot
            *snapshot;                   /* Immutable borrowed bytes, alive until quiescence. */
        FVM_SWFStreamSoundInfo info;     /* Validated copied source geometry. */
        size_t block_count, block_index; /* Indexed segment extent and current source block. */
        size_t first; /* Next source frame, or bounded downsampling carry into the next block. */
        unsigned long phase;       /* Rational fractional position, always less than output_rate. */
        unsigned long output_rate; /* Chosen output Hz, MIN_RATE..MAX_RATE. */
        unsigned int output_channels; /* One or two scalar values per output frame. */
        int continuous; /* Nonzero selects adjacent samples across prepared block boundaries. */
        int started; /* Whether a chunk of the current block has already been returned. */
    } FVM_SWFStreamConverter;
    /* Borrow a PCM snapshot without allocation; init rejects other codecs/host representations
     * through reader_init. Output rate/channels explicit; errors preserve caller state. */
    FVM_API FVM_Result fvm_swf_stream_converter_init(FVM_SWFStreamConverter *,
                                                     const FVM_SWFStreamSoundSnapshot *,
                                                     unsigned long output_rate,
                                                     unsigned int channels);
    /* Like init, but interpolate across nonempty blocks of this immutable segment. Empty
     * blocks and movie-frame gaps add no samples. Only segment EOF holds the final sample.
     * No allocation or VM access; snapshot lifetime and single-owner restrictions match init.
     * Errors preserve state. Rebuild callers: the state descriptor has gained a policy field. */
    FVM_API FVM_Result fvm_swf_stream_converter_init_continuous(FVM_SWFStreamConverter *,
                                                     const FVM_SWFStreamSoundSnapshot *,
                                                     unsigned long output_rate,
                                                     unsigned int channels);
    /* Read at most min(max_frames,MAX_FRAMES) without crossing an encoded block. Positive max
     * required; capacity/written count scalar S16 values. Errors/EOF preserve all outputs/state.
     * Linear interpolation holds the last sample inside each block; it never borrows the next
     * block's value unless initialized with init_continuous. Rational phase and downsampling carry survive block boundaries/empty blocks.
     * Mono duplicates; stereo->mono averages toward zero before interpolation/truncation toward
     * zero. Empty/skipped blocks return OK zero-value markers and advance. No allocation, VM,
     * callback, device, clock, anti-alias filter or gap synthesis; all buffers/state disjoint. */
    FVM_API FVM_Result fvm_swf_stream_converter_next(FVM_SWFStreamConverter *, size_t max_frames,
                                                     short *, size_t capacity,
                                                     FVM_SWFStreamConvertedChunk *,
                                                     size_t *written);
    /* Set a source block/offset (<= actual count); phase becomes zero and starts a new conversion
     * grid at that source point. block_count/0 seeks EOF. Does not rewind queued host audio. */
    FVM_API FVM_Result fvm_swf_stream_converter_seek(FVM_SWFStreamConverter *, size_t block,
                                                     size_t first);
#ifdef __cplusplus
}
#endif
#endif
