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

Set [FVM_SWFStreamSoundSeekPoint](../include/flashvm/swf_stream_sound.h#L175).struct_size to its exact size, then call
[fvm_swf_stream_sound_snapshot_seek_point](../include/flashvm/swf_stream_sound.h#L196) with a positive one-based movie frame.
The first authored block at/after it determines source_frame. The resolved
block_index and first select normalized source PCM, potentially in another block.
PCM/ADPCM use offset zero. No block at/after the request returns EOF with
[FVM_SWF_STREAM_SEEK_EOF_FRAME](../include/flashvm/swf_stream_sound.h#L171); exact PCM end retains the selected authored frame.
Negative/past-end target, null or wrong-size output and frame zero return
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47); encoded compressed data returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48);
arithmetic overflow returns [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L46). Failed queries preserve output.
No allocation, decoding, callback, script or GC access occurs. Independent const
reads can share a post-VM snapshot; synchronize close separately. The host owns
source authorization, resampler phase reset and device queue transactions.


## Transactional output

[fvm_swf_stream_output_bind_frame_aligned](../include/flashvm/swf_stream_output.h#L210) validates converter geometry and the
point before callbacks; [fvm_swf_stream_output_seek_frame_aligned](../include/flashvm/swf_stream_output.h#L215) uses the current
converter. Both reuse pause → clear → publish. Only successful clear publishes
the new cursor/binding and resets pending PCM, counters and resampler phase.
A failed clear retains old PCM/cursors with the acknowledged pause still visible.
Callback statuses propagate; exceptions become [FVM_ERROR](../include/flashvm/flashvm.h#L45) and busy reentry is
rejected. No allocation/decode/VM/GC operation occurs. Keep snapshot, converter,
backend and output storage alive and serialize mutations. Success remains paused.
Legacy bind_frame/seek_frame continue selecting the authored block start.


## Desktop root integration

Nonsequential root navigation now uses aligned binding under the audio mutex.
The owner publishes only scalars after querying the VM. A selected future authored
block cannot play inside the preceding visual gap, even with a backward signed
position. Once that authored frame is authorized, its resolved starting PCM block
can play even if positive alignment/initial trimming moved it to a later block.
Only this starting block is promoted; later blocks keep the existing source gate.
Authored Stop, host pause and incomplete frame callbacks still withhold permission.
Continuous playback does not reapply SeekSamples. Full reference read-ahead,
source-time scheduling and A/V drift correction remain open.


## API and execution boundaries

| Operation | Result and ownership |
| --- | --- |
| [fvm_swf_stream_sound_snapshot_seek_point](../include/flashvm/swf_stream_sound.h#L196) | Copies a position; never changes snapshot or output on error. Caller supplies exact structure size and retains snapshot. |
| [fvm_swf_stream_output_bind_frame_aligned](../include/flashvm/swf_stream_output.h#L210) | Validates another converter, pauses and clears backend, then publishes the binding. Both converters remain owned by host. |
| [fvm_swf_stream_output_seek_frame_aligned](../include/flashvm/swf_stream_output.h#L215) | Applies the same transaction to the currently bound converter. |
| [fvm_swf_stream_pcm_reader_seek](../include/flashvm/swf_stream_pcm.h#L62) | Applies the returned block_index/first to an offline native reader; no queue effects. |
| [fvm_swf_stream_converter_seek](../include/flashvm/swf_stream_convert.h#L83) | Applies source position and resets rational phase to zero; native host owns synchronization. |

Source offsets count sample frames/pairs, not interleaved channel values or output
frames. Queries are logarithmic in indexed blocks. MP3 normalization retains all
PCM and metadata in its existing bounded native owner; sizeof the private index
is included in runtime and explicit peak charges. No public existing struct grows.
A copied seek point owns no snapshot pointer: it is meaningful only for its source.
Do not mix it with another segment or close the owner while readers/workers run.

When SeekSamples puts the position outside prepared PCM, reject the seek; do not
clamp or invent silence. A failed backend must obey its all-or-none callback
contract. Rejected writes retain exact pending bytes. A successful seek cancels
those bytes only after clear acknowledgement, including through mixed adapters.
This is finite prepared-buffer positioning, not decoder restart in arbitrary
truncated MPEG data or proof of complete Flash player equivalence.

## Reproducible checks

Build the `flashvm_tests_swf_stream_mp3_seek_point`,
`flashvm_tests_swf_stream_mp3_seek_output`,
`flashvm_tests_swf_stream_mp3_seek_allocation` and
`flashvm_tests_swf_stream_mp3_seek_threads` targets. With an explicit SDL2 SDK and
desktop frontend, also build `flashvm_frontend_stream_mp3_seek`. The C89 tutorial
`flashvm_examples_multimedia_swf_stream_mp3_main --seek INPUT FRAME OUTPUT`
exports a post-VM suffix. `scripts/check_mp3_seek_example.py EXECUTABLE` compares
whole suffixes to FFmpeg; missing FFmpeg/ffprobe explicitly skips with status 77.
These are native/dummy/analytical tests, separate from physical device acceptance.


## Validation checkpoint

Prepared authored MP3 seeks v0.1077–v0.1089: native snapshots retain original decoded prefixes and signed block adjustments; additive immutable position and transactional output APIs apply authored offsets only on discontinuities. Desktop root navigation preserves selected-frame authorization, including backward starts across visual gaps and latency-shifted starting blocks. Existing block-start APIs and continuous PCM retain their behavior. Validation passes 241 selected native regression/tutorial/CLI checks, five selected full-profile ASan/UBSan checks plus explicit no-throw output and no-fault allocation runs, and nine SDK-free checks. Ordinary builds cover callback exceptions and all four allocation failures; LeakSanitizer is disabled. Six MPEG vectors exercise signed bounds, two latencies, empty markers, exact end, unchanged failure outputs and ordinary PCM fallback. Four OS workers check 256 post-VM epochs; zero-allocation controls check 128 epochs. Independent FFmpeg compares 192 complete C89-exported suffixes across FWS/CWS, signed offsets, latency and EOF. Real SDL dummy queues verify actual MovieClip interpreter/native-JIT execution and GC; a running worker submits nothing before a selected future authored frame and resumes after authorization. A renamed detached SDK-free component configures and its copied oracle passes against the native exporter; no separate detached build. Negative initial latency, arbitrary MPEG decoder restart, complete reference read-ahead/source-time A/V drift, child timelines/dictionaries, physical devices/foreign platforms and all nine roadmap stages remain open. [Contract](MP3_FRAME_SEEK.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `fvm_swf_stream_converter_seek` | function | [include/flashvm/swf_stream_convert.h:83](../include/flashvm/swf_stream_convert.h#L83) |
| `fvm_swf_stream_output_bind_frame_aligned` | function | [include/flashvm/swf_stream_output.h:210](../include/flashvm/swf_stream_output.h#L210) |
| `fvm_swf_stream_output_seek_frame_aligned` | function | [include/flashvm/swf_stream_output.h:215](../include/flashvm/swf_stream_output.h#L215) |
| `fvm_swf_stream_pcm_reader_seek` | function | [include/flashvm/swf_stream_pcm.h:62](../include/flashvm/swf_stream_pcm.h#L62) |
| `FVM_SWF_STREAM_SEEK_EOF_FRAME` | macro / define | [include/flashvm/swf_stream_sound.h:171](../include/flashvm/swf_stream_sound.h#L171) |
| `fvm_swf_stream_sound_snapshot_seek_point` | function | [include/flashvm/swf_stream_sound.h:196](../include/flashvm/swf_stream_sound.h#L196) |
| `FVM_SWFStreamSoundSeekPoint` | struct | [include/flashvm/swf_stream_sound.h:175](../include/flashvm/swf_stream_sound.h#L175) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
