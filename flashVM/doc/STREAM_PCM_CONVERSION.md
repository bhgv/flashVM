# Explicit prepared stream PCM conversion

[Русский](STREAM_PCM_CONVERSION.ru.md), [native reader](STREAM_PCM_READER.md).
`swf_stream_convert.h` provides a caller-owned, allocation-free C89 converter over
an immutable prepared PCM segment. It prepares native S16 for a chosen sample
clock/channel count without consulting a live VM, device or wall clock.

## Exact position and block policy

Output rate is explicit, 5512..192000 Hz; channels are mono or stereo. Source
rate/channels are actual stream geometry, never advisory playback fields.
The grid advances by source_rate/output_rate using integer quotient/remainder.
Fractional phase and source-frame overshoot carry across encoded blocks and empty
blocks, preventing independent per-block rounding from accumulating duration drift.
Across N source frames, output count is ceil(N*output_rate/source_rate), without
multiplying unbounded lengths in the implementation. Movie-frame gaps are metadata;
this converter neither inserts silence nor implements a playback timeline.

Interpolation is linear between adjacent samples inside the same encoded block.
At its last source sample, hold that value through its remaining source interval;
do not interpolate against a different block or across an authored frame gap.
This explicit boundary policy is deterministic, not a band-limited resampler.
There is no anti-alias filter, codec, A/V synchronization, queue or device scheduling.
The desktop player's unmuted root stream playback remains unsupported.

Mono duplicates into stereo. Stereo-to-mono averages toward zero before temporal
interpolation. Final interpolation truncates toward zero. Double products remain
exact below 2^53, avoiding 32-bit long overflow on ILP32/LLP64. Only PCM_NATIVE and
PCM_LE segments accepted by the existing reader are supported.

Each next call visits one encoded block, emits at most min(max_frames,1024), and
preserves stream/block/movie-frame identity. Capacity/written count scalar channel
values; chunk.frames counts output frames. first_source/phase describe the starting
source grid point. BEGIN means the first returned chunk/marker of this block;
END means no more output positions belong to it. Empty blocks return zero values
with EMPTY. Downsampling may skip a nonempty block entirely: it still returns a
zero-value BEGIN|END marker without EMPTY and carries the overshoot forward.
Every OK advances output or block position. EOF is NOT_FOUND, preserving outputs.

## API and ownership

| API | Contract |
| --- | --- |
| [`fvm_swf_stream_converter_init`](../include/flashvm/swf_stream_convert.h#L47) | Validate explicit output rate/channels and borrow a prepared native PCM snapshot; copy source metadata/count. INVALID_ARGUMENT for bad args, UNSUPPORTED for codecs/host representation. No allocation; errors preserve state. |
| [`fvm_swf_stream_converter_next`](../include/flashvm/swf_stream_convert.h#L58) | Positive max_frames; exact chunk.struct_size; enough scalar capacity and nonnull PCM for nonempty results. Bounded planning validates capacity before output. EOF/errors preserve PCM/chunk/written/state. No VM, I/O, allocation, callbacks or implicit retry. |
| [`fvm_swf_stream_converter_seek`](../include/flashvm/swf_stream_convert.h#L64) | Set a valid block/source offset <= actual sample_count; block_count/0 is EOF. Start a new grid with phase zero. Invalid positions preserve state. Already queued/played output is not rewound. |

Snapshot ownership stays with the host until every converter/reader is quiesced.
Source runtime/context may be destroyed after snapshot preparation. Serialize each
converter, use disjoint state/input/output storage; independent converters may
share immutable native bytes. State has no self-references and may be copied for
position snapshots while preserving the same borrowed-source lifetime.

## Validation

Native tests use variable one/two/three-frame blocks, explicit empty blocks,
seven output rates including 48000 and 192000, mono/stereo and multiple chunk sizes.
An independent integer long-division interpolation oracle avoids native floating
point and 64-bit integer extensions. Tests cover rational duration, skipped blocks,
EOF/seek/capacity atomicity, max-work clamping and C89 linkage. Allocation failure
injection forbids new allocation during conversion after original VM teardown.

```sh
cmake -S flashVM -B build-convert -DFVM_BUILD_TESTS=ON
cmake --build build-convert --target flashvm_tests_swf_stream_converter \
  flashvm_tests_swf_stream_pcm_reader_api flashvm_tests_swf_stream_sound_snapshot_allocation
ctest --test-dir build-convert -R 'swf_stream_(converter|pcm_reader_api|sound_snapshot_allocation)$' --output-on-failure
```

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_converter_init` | function | [include/flashvm/swf_stream_convert.h:47](../include/flashvm/swf_stream_convert.h#L47) |
| `fvm_swf_stream_converter_next` | function | [include/flashvm/swf_stream_convert.h:58](../include/flashvm/swf_stream_convert.h#L58) |
| `fvm_swf_stream_converter_seek` | function | [include/flashvm/swf_stream_convert.h:64](../include/flashvm/swf_stream_convert.h#L64) |
<!-- source-links:end -->
