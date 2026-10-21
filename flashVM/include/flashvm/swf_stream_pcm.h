/* Sequential frame-aware PCM extraction from immutable prepared SWF sound segments. */
#ifndef FLASHVM_SWF_STREAM_PCM_H
#define FLASHVM_SWF_STREAM_PCM_H /* Guard the allocation-free C89 reader ABI. */
#include "swf_stream_sound.h"
#define FVM_STREAM_PCM_BEGIN 1U /* Chunk starts at the encoded block's first sample frame. */
#define FVM_STREAM_PCM_END 2U   /* Chunk reaches the encoded block's end. */
#define FVM_STREAM_PCM_EMPTY 4U /* Explicit zero-length encoded block, not an invented frame gap.  \
                                 */
#ifdef __cplusplus
extern "C"
{
#endif
    /* One owned metadata copy; PCM is written separately into the caller's bounded buffer. */
    typedef struct FVM_SWFStreamPCMChunk
    {
        size_t struct_size;  /* Caller sets sizeof(FVM_SWFStreamPCMChunk). */
        size_t stream_index; /* Source header occurrence, never a runtime pointer. */
        size_t block_index;  /* Encoded block ordinal within that prepared segment. */
        unsigned int frame;  /* One-based movie frame; gaps are preserved, never synthesized. */
        size_t first;        /* Source sample-frame offset within the encoded block. */
        size_t frames;       /* Complete mono frames or stereo pairs in this chunk. */
        unsigned int sample_rate; /* Actual source rate; playback advice is not resampling. */
        unsigned int channels;    /* Source interleaving, one or two channel values per frame. */
        unsigned int flags; /* BEGIN/END/EMPTY describe this chunk's encoded block boundaries. */
    } FVM_SWFStreamPCMChunk;
    /* Caller-owned serial cursor; no allocation, mutable globals or managed roots. */
    typedef struct FVM_SWFStreamPCMReader
    {
        size_t struct_size; /* Exact ABI initialized by reader_init. */
        const FVM_SWFStreamSoundSnapshot
            *snapshot;               /* Borrowed immutable native source until disposal. */
        FVM_SWFStreamSoundInfo info; /* Copied validated source geometry. */
        size_t block_count;          /* Prepared segment's exact indexed block count. */
        size_t block_index;          /* Next encoded block, block_count means EOF. */
        size_t first;                /* Next sample frame within that block. */
    } FVM_SWFStreamPCMReader;
    /* Bind a prepared PCM_NATIVE/PCM_LE snapshot without ownership transfer or allocation.
     * Source must outlive reader use; original context/runtime need not exist. Other codecs or
     * unsupported host-short representation return UNSUPPORTED. Errors preserve reader. */
    FVM_API FVM_Result fvm_swf_stream_pcm_reader_init(FVM_SWFStreamPCMReader *,
                                                      const FVM_SWFStreamSoundSnapshot *);
    /* Read up to max_frames (>0), never cross an encoded block boundary. capacity/written count
     * channel values, chunk.frames counts sample frames. Explicit empty blocks return OK with
     * zero values and EMPTY|BEGIN|END; EOF returns NOT_FOUND. Successful reads always advance.
     * INVALID_ARGUMENT for bad descriptors/ranges/capacity; every error preserves cursor, chunk,
     * PCM and written. No allocation, callback, I/O, VM access, timing or resampling. Disjoint
     * caller inputs/outputs, one owner per cursor; independent readers may share the snapshot. */
    FVM_API FVM_Result fvm_swf_stream_pcm_reader_next(FVM_SWFStreamPCMReader *, size_t max_frames,
                                                      short *pcm, size_t capacity,
                                                      FVM_SWFStreamPCMChunk *, size_t *written);
    /* Seek to an indexed block and sample offset <= its actual sample_count. End offsets produce
     * a zero-value END marker on next; an empty block additionally has EMPTY. block_count/0 is
     * explicit EOF. Invalid positions preserve cursor. No decoder/output side effects. */
    FVM_API FVM_Result fvm_swf_stream_pcm_reader_seek(FVM_SWFStreamPCMReader *, size_t block,
                                                      size_t first);
#ifdef __cplusplus
}
#endif
#endif
