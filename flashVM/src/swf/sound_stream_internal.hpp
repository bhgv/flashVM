/* Native stream snapshot ownership shared by encoded inspection and codec preparation. */
#ifndef FVM_SWF_SOUND_STREAM_INTERNAL_HPP
#define FVM_SWF_SOUND_STREAM_INTERNAL_HPP /* Guard private native snapshot storage. */
#include "flashvm/swf_stream_sound.h"
#include "../details/vector.hpp"
/* Owned block plus source timing and its offset into immutable payload storage. */
struct FVM_SWFStreamSoundEntry
{
    FVM_SWFStreamSoundBlock block; /* Copied wire metadata; data is reconstructed on lookup. */
    size_t mp3_decoded_first; /* Original decoded frame prefix before initial latency trimming. */
    int mp3_seek_samples; /* Signed authored adjustment, used only for discontinuous seeks. */
    size_t offset;                 /* Byte offset in owned encoded storage or normalized native PCM. */
};
/* [GC:] Native independent snapshot; no VM pointer or managed value is retained. */
struct FVM_SWFStreamSoundSnapshot
{
    FVM_SWFStreamSoundInfo info;                        /* Copied source and playback header. */
    fvm_detail::vector<FVM_SWFStreamSoundEntry> blocks; /* Exact segment block index. */
    fvm_detail::vector<unsigned char> bytes; /* Original wire bytes, not expanded samples. */
    fvm_detail::vector<short> pcm; /* Optional expanded native-S16 storage, aligned for decoding. */
    size_t retained_bytes; /* Snapshot object plus its current payload/index allocation sizes. */
    size_t peak_bytes;     /* Maximum simultaneous raw/decoded ownership during preparation. */
    bool prepared_mp3; /* Complete MP3 preparation retained seek metadata in normalized PCM. */
    unsigned char empty;   /* Valid address for an entirely empty payload store. */
};

/* Consume an owned encoded MP3 snapshot; close it on failure, publish independent PCM on OK. */
FVM_Result fvm_swf_stream_prepare_mp3(FVM_SWFStreamSoundSnapshot *, size_t runtime_limit,
                                      const size_t *peak_budget, FVM_SWFStreamSoundSnapshot **);
#endif
