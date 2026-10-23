# Prepared MP3 frame seeks

MP3 preparation retains each block's original decoded prefix and signed
SeekSamples before discarding encoded bytes. Preparation performs two complete
passes with a persistent reservoir per pass; the resulting PCM trims initial
latency once. The immutable native owner survives VM destruction.

For a later authored block, let R be its original decoded prefix and S its signed
SeekSamples. Adobe's SWF specification (pages 185–187) adds initial latency L
on discontinuous seeks: raw position R+L+S. In the already trimmed snapshot this
is R+S. The first block starts at normalized zero. Continuous playback remains
unchanged. Reference: [Adobe SWF specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf).

Negative initial latency, full reference A/V scheduling, decoder read-ahead,
child streams and physical device acceptance remain open. All nine roadmap
stages remain open. Version numbers do not certify SWF 8 completeness.

## Immutable position query

Set [FVM_SWFStreamSoundSeekPoint](../include/flashvm/swf_stream_sound.h#L142).struct_size to its exact size, then call
[fvm_swf_stream_sound_snapshot_seek_point](../include/flashvm/swf_stream_sound.h#L161) with a positive one-based movie frame.
The first authored block at/after it determines source_frame. The resolved
block_index and first select normalized source PCM, potentially in another block.
PCM/ADPCM use offset zero. No block at/after the request returns EOF with
[FVM_SWF_STREAM_SEEK_EOF_FRAME](../include/flashvm/swf_stream_sound.h#L140); exact PCM end retains the selected authored frame.
Negative/past-end target, null or wrong-size output and frame zero return
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); encoded compressed data returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46);
arithmetic overflow returns [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44). Failed queries preserve output.
No allocation, decoding, callback, script or GC access occurs. Independent const
reads can share a post-VM snapshot; synchronize close separately. The host owns
source authorization, resampler phase reset and device queue transactions.


## Transactional output

[fvm_swf_stream_output_bind_frame_aligned](../include/flashvm/swf_stream_output.h#L176) validates converter geometry and the
point before callbacks; [fvm_swf_stream_output_seek_frame_aligned](../include/flashvm/swf_stream_output.h#L179) uses the current
converter. Both reuse pause → clear → publish. Only successful clear publishes
the new cursor/binding and resets pending PCM, counters and resampler phase.
A failed clear retains old PCM/cursors with the acknowledged pause still visible.
Callback statuses propagate; exceptions become [FVM_ERROR](../include/flashvm/flashvm.h#L43) and busy reentry is
rejected. No allocation/decode/VM/GC operation occurs. Keep snapshot, converter,
backend and output storage alive and serialize mutations. Success remains paused.
Legacy bind_frame/seek_frame continue selecting the authored block start.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_swf_stream_output_bind_frame_aligned` | function | [include/flashvm/swf_stream_output.h:176](../include/flashvm/swf_stream_output.h#L176) |
| `fvm_swf_stream_output_seek_frame_aligned` | function | [include/flashvm/swf_stream_output.h:179](../include/flashvm/swf_stream_output.h#L179) |
| `FVM_SWF_STREAM_SEEK_EOF_FRAME` | macro / define | [include/flashvm/swf_stream_sound.h:140](../include/flashvm/swf_stream_sound.h#L140) |
| `fvm_swf_stream_sound_snapshot_seek_point` | function | [include/flashvm/swf_stream_sound.h:161](../include/flashvm/swf_stream_sound.h#L161) |
| `FVM_SWFStreamSoundSeekPoint` | struct | [include/flashvm/swf_stream_sound.h:142](../include/flashvm/swf_stream_sound.h#L142) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
