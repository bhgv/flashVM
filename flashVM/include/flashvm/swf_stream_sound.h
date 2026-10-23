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
    /* Owner-only encoded copy with an explicit object/index/payload peak-byte ceiling.
     * Preserve codec bytes and metadata; never decode or validate compressed codec payloads.
     * Same source validation and runtime payload limit as snapshot_open. Clear *out first;
     * zero/insufficient budget returns OUT_OF_MEMORY before allocating the snapshot.
     * Allocation failure frees temporary ownership. No callback/device/VM execution.
     * Immutable result survives source teardown; use existing block/MP3 views and close. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_open_bounded(
        FVM_Context *, size_t stream, size_t budget_bytes, FVM_SWFStreamSoundSnapshot **);
    /* Owner-only preparation: copy PCM or expand whole ADPCM packets / continuous MP3 data.
     * ADPCM output is normalized PCM_NATIVE/S16 with exact per-block counts; source/frame/header
     * identity and advisory playback fields persist. Zero-byte/width-only empty blocks allowed.
     * MP3: retain one MPEG reservoir across complete packets/blocks; require exact per-block
     * declared sample counts and stable effective geometry, trim header latency exactly once.
     * First block seek must match header latency; later seek fields do not trim continuous PCM.
     * Negative initial latency/free-format MPEG/other codecs and partial inferred ADPCM
     * UNSUPPORTED; malformed/truncated MPEG, count/geometry mismatch or padding INVALID_ARGUMENT.
     * MP3 normalized block counts describe post-latency PCM; frame identities persist. This
     * does not add reference Flash discontinuous MP3 seek or decoder-read-ahead scheduling.
     * Output cleared first; encoded+decoded payload/index must fit runtime memory limit.
     * May allocate/decode; failure releases temporary ownership. No callback or device/VM execution.
     * Returned immutable snapshot uses existing reader/converter/close APIs after VM teardown. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_open_pcm(FVM_Context *, size_t,
                                                              FVM_SWFStreamSoundSnapshot **);

    /* Native allocation charges, excluding allocator bookkeeping, stack, VM and device memory. */
    typedef struct FVM_SWFStreamSoundStorageInfo
    {
        size_t struct_size; /* Caller sets exact sizeof this native observation structure. */
        size_t retained_bytes; /* Snapshot object, exact block index and retained raw/PCM arrays. */
        size_t peak_bytes; /* Largest simultaneous object/index/encoded/decoded charge during open. */
    } FVM_SWFStreamSoundStorageInfo;
    /* Same owner/preparation/codec contract as open_pcm, plus an explicit peak-byte ceiling.
     * Zero is a valid empty budget: even an empty snapshot needs ownership and returns OOM.
     * Runtime payload/index limit still applies independently. Output cleared first; budget
     * excess OUT_OF_MEMORY before the offending allocation. No device/callback/VM execution.
     * Subtract retained_bytes from an aggregate budget before preparing the next snapshot. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_open_pcm_bounded(
        FVM_Context *, size_t stream, size_t budget_bytes, FVM_SWFStreamSoundSnapshot **);
    /* Owner-only bounded ADPCM expansion using host-authoritative per-block frame counts.
     * counts has exactly count entries in encoded block order (including empty blocks).
     * It may be null only for count=0; table storage is borrowed until return, never retained.
     * Unlike open_pcm, partial final packets are supported; average_samples is never used.
     * Exact encoded length and zero final padding are required. No alignment between packets.
     * Zero frames accepts an empty payload or a width-only byte with zero padding.
     * Count/length/padding mismatch or frames > ULONG_MAX INVALID_ARGUMENT; non-ADPCM
     * UNSUPPORTED; overflow/allocation/runtime or explicit peak ceiling OUT_OF_MEMORY.
     * Clear *out first; null out INVALID_ARGUMENT. Failure releases all temporary ownership.
     * Same storage charges, native lifetime, read/converter/close and external serialization
     * contracts as open_pcm_bounded. May allocate/decode; no callback/device/VM execution.
     * A count is an explicit host assertion, not inferred/verified movie timing; multiple
     * counts can fit identical zero-padded bytes. Do not guess counts from a header average. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_open_adpcm_counted(
        FVM_Context *, size_t stream, const size_t *counts, size_t count,
        size_t budget_bytes, FVM_SWFStreamSoundSnapshot **);
    /* Copy immutable native charges without allocation/callback/VM; valid after VM teardown.
     * Null/wrong-size INVALID_ARGUMENT, output intact. Caller retains snapshot until return;
     * concurrent const reads allowed; synchronize close separately. Peak >= retained. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_storage_info(
        const FVM_SWFStreamSoundSnapshot *, FVM_SWFStreamSoundStorageInfo *);

    /* Bounded PCM preparation with explicit ADPCM length inference. Preserve legacy canonical
     * whole-packet policy first; otherwise inspect all zero-padding endpoints (0..7 bits).
     * Exactly one valid packet geometry accepts a partial final packet; multiple -> UNSUPPORTED,
     * none -> INVALID_ARGUMENT. Header average/movie time never choose a count. Other codecs,
     * memory limits, ownership and errors match open_pcm_bounded. Required out is cleared first.
     * Preparation allocates/decodes synchronously on context owner; no callbacks/script/GC roots
     * retained. Native immutable snapshot can outlive VM; close after reader/worker quiescence.
     * Encoded bytes/index/decoded peak fit budget; overflow/allocation -> OUT_OF_MEMORY. Invalid
     * source/missing stream statuses propagate. Existing whole-only APIs remain unchanged.
     * This is a byte-geometry policy, not proof of an author's intended trailing zero codes. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_open_pcm_inferred(
        FVM_Context *, size_t stream, size_t budget_bytes, FVM_SWFStreamSoundSnapshot **out);

    /* No authored block at/after the requested movie frame. */
#define FVM_SWF_STREAM_SEEK_EOF_FRAME 0U
    /* Copied position in normalized PCM; no borrowed storage or VM values. */
    typedef struct FVM_SWFStreamSoundSeekPoint
    {
        size_t struct_size; /* Required exact sizeof this structure. */
        size_t block_index; /* Actual PCM block; block_count represents EOF. */
        size_t first; /* Source sample-frame offset within the resolved PCM block. */
        unsigned int source_frame; /* Selected authored frame, or SEEK_EOF_FRAME when absent. */
        int mp3_aligned; /* Nonzero only for completely prepared MP3. */
    } FVM_SWFStreamSoundSeekPoint;
    /* Resolve the first authored block at/after positive frame, applying signed MP3 alignment.
     * Prepared MP3 uses original decoded prefix + later block SeekSamples in normalized PCM;
     * initial block starts at zero because initial latency was already removed. PCM/ADPCM
     * select block offset zero. Actual PCM may lie in an earlier/later block; source_frame
     * retains the selected authored frame, including when its position equals exact EOF.
     * Beyond last authored block succeeds with block_count/0 and SEEK_EOF_FRAME.
     * Null/wrong struct_size/zero frame or negative/past-end target INVALID_ARGUMENT;
     * compressed encoded snapshots UNSUPPORTED; arithmetic overflow OUT_OF_MEMORY.
     * Output unchanged on failure. No allocation, decoding, callbacks, VM access or GC roots.
     * Immutable concurrent reads allowed after VM teardown; retain owner and synchronize close.
     * This computes a source position only: hosts retain frame authorization/A-V scheduling. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_seek_point(
        const FVM_SWFStreamSoundSnapshot *, unsigned int frame, FVM_SWFStreamSoundSeekPoint *);

    /* Copy the snapshot header (normalized after open_pcm ADPCM), preserving output on error. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_info(const FVM_SWFStreamSoundSnapshot *, FVM_SWFStreamSoundInfo *);
    /* Query the prepared block count without rescanning the movie. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_block_count(const FVM_SWFStreamSoundSnapshot *, size_t *);
    /* Borrow indexed payload until close: original bytes, or native S16 after open_pcm ADPCM. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_block_at(const FVM_SWFStreamSoundSnapshot *, size_t, FVM_SWFStreamSoundBlock *);
    /* Inspect the indexed MP3 prefix after source VM teardown; the MPEG span lasts until close. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_mp3_block_at(const FVM_SWFStreamSoundSnapshot *, size_t, FVM_SWFStreamSoundMP3Block *);
    /* Decode a requested range to native S16; written counts channel values and is cleared first. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_read_pcm(const FVM_SWFStreamSoundSnapshot *, size_t, size_t, size_t, short *, size_t, size_t *);
    /* Decode one ADPCM block range using an authoritative total source-frame count.
     * Live form scans on the context owner; snapshot form needs an ENCODED ADPCM snapshot,
     * may outlive the VM and permits concurrent immutable reads into separate output buffers.
     * samples is the exact total for this block, first/frames select its bounded range.
     * Validate full length/padding and all declared codes before publishing any PCM, including
     * for zero-length reads. Zero samples accepts empty/width-only blocks. Average is ignored.
     * out holds aligned native S16, capacity/written count scalar channel values, not frames.
     * Positive frames require out and enough capacity; written is required and cleared first.
     * Output storage, written and source must be disjoint. No PCM changes on failure.
     * INVALID_ARGUMENT for bad pointers/range/length/padding or samples > ULONG_MAX;
     * UNSUPPORTED for non-ADPCM/unsupported native S16 representation; arithmetic overflow
     * OUT_OF_MEMORY; source/missing block statuses propagate. No allocation/callback/VM
     * execution or mutation. Runtime is linear in full declared block length, not range length;
     * repeated tiny reads revalidate from the beginning. Use counted preparation for playback.
     * Keep context/snapshot alive and bytes immutable until return; serialize live changes
     * and synchronize snapshot close. No managed pointer/GC root or output owner is retained. */
    FVM_API FVM_Result fvm_swf_stream_sound_read_adpcm(
        FVM_Context *, size_t stream, size_t block, size_t samples, size_t first,
        size_t frames, short *out, size_t capacity_values, size_t *written_values);
    /* Encoded-snapshot counterpart of read_adpcm; same range/error/output contract above. */
    FVM_API FVM_Result fvm_swf_stream_sound_snapshot_read_adpcm(
        const FVM_SWFStreamSoundSnapshot *, size_t block, size_t samples, size_t first,
        size_t frames, short *out, size_t capacity_values, size_t *written_values);
    /* Free the independent snapshot; null is a no-op. */
    FVM_API void fvm_swf_stream_sound_snapshot_close(FVM_SWFStreamSoundSnapshot *);
#ifdef __cplusplus
}
#endif
#endif
