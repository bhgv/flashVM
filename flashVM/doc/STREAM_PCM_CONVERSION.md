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
The desktop player supports the prepared root-stream codec subset described below.

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
| [`fvm_swf_stream_converter_init`](../include/flashvm/swf_stream_convert.h#L48) | Validate explicit output rate/channels and borrow a prepared native PCM snapshot; copy source metadata/count. INVALID_ARGUMENT for bad args, UNSUPPORTED for codecs/host representation. No allocation; errors preserve state. |
| [`fvm_swf_stream_converter_next`](../include/flashvm/swf_stream_convert.h#L67) | Positive max_frames; exact chunk.struct_size; enough scalar capacity and nonnull PCM for nonempty results. Bounded planning validates capacity before output. EOF/errors preserve PCM/chunk/written/state. No VM, I/O, allocation, callbacks or implicit retry. |
| [`fvm_swf_stream_converter_seek`](../include/flashvm/swf_stream_convert.h#L73) | Set a valid block/source offset <= actual sample_count; block_count/0 is EOF. Start a new grid with phase zero. Invalid positions preserve state. Already queued/played output is not rewound. |

Snapshot ownership stays with the host until every converter/reader is quiesced.
Source runtime/context may be destroyed after snapshot preparation. Serialize each
converter, use disjoint state/input/output storage; independent converters may
share immutable native bytes. State has no self-references and may be copied for
position snapshots while preserving the same borrowed-source lifetime.

## Validation

Native tests cover all four nominal source rates and variable one/two/three-frame blocks, explicit empty blocks,
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

## Native C export and workers (v0.752)

The [annotated swf_stream tutorial](../examples/multimedia/swf_stream/README.md)
accepts `OUTPUT.pcm --convert`. It copies the source segment, destroys its original
runtime/context and exports 48000 Hz stereo S16 using direct C file callbacks.
No replacement output VM is created. The three source frames in two nonempty
encoded blocks produce four output frames; the intervening movie-frame gap is
not filled, and the explicit empty block remains visible. First per-block report
lines describe source lengths; the final exported count describes output frames.
The unchanged default/`--prepared`/`--cursor` modes still emit native-rate samples.

The independent Python test computes the converted samples with integer weighted
sums/truncation and compares every output byte. FFmpeg remains an independent
oracle for original U8-to-S16 decoding, not for this converter's explicit block-edge
policy. Failed file opens and replacement ownership are exercised in all modes.
Four real native workers additionally perform 500 conversion rewind/read/EOF
passes each, at 5512/48000/96000/192000 Hz, while sharing only immutable source bytes.
Main joins every worker before source close. This proves a bounded Linux native
thread scenario, not foreign OS/device or real-time audio callback acceptance.

```sh
cmake -S flashVM -B build-stream -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-stream --target flashvm_examples_multimedia_swf_stream_main
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --convert
python3 flashVM/scripts/check_swf_stream_example.py ./build-stream/flashvm_examples_multimedia_swf_stream_main
```

Validation at v0.752: 73 selected native regression checks pass (two resource-player launches were rerun after restoring the built executable permission), plus 14 shared-host checks. New converter/reader/snapshot paths, native worker tests and all file-export modes pass ASan/UBSan; whole-VM instrumentation and leak detection are not claimed (host disables leak detection). New implementation and C89 consumer/tutorial objects compile for ARM64 Linux/musl, RISC-V64 Linux/musl and Windows x64/MinGW; foreign binaries and full cross-player builds are not executed. All nine roadmap stages remain open.
[Bounded queue producer / Ограниченный producer очереди](STREAM_PCM_OUTPUT.md).


## Continuous segment interpolation (v0.803)

[`fvm_swf_stream_converter_init_continuous`](../include/flashvm/swf_stream_convert.h#L56) has the same arguments, statuses, borrowed
snapshot lifetime and single-owner/native-thread contract as init. It selects linear
interpolation against the first sample of the next nonempty block. Only segment EOF
holds the final value. Movie-frame gaps and empty blocks contain no implicit silence.
No allocation, callbacks, VM access or additional PCM storage is needed. A boundary
lookup visits intervening empty descriptors once per next call; worst-case work includes
the remaining block index, in addition to the bounded 1024 output frames. Output chunks
retain the left sample's block identity; lookahead does not authorize or emit a future
block. Seek resets phase and retains the selected policy. Legacy init retains clamping.
The caller-owned converter descriptor gained a field: rebuild all native callers.

The integer oracle covers both policies, all four SWF PCM rates, seven output rates,
mono/stereo mapping, one/eleven-frame chunks, empty blocks, final clamping and seek.
This is native deterministic evidence, not audible device acceptance or full SWF support.


## Desktop use (v0.804)

The SDL player now selects continuous interpolation independently for each prepared
root-stream segment. PCM, ADPCM and MP3 use the same normalized S16 path. Header
changes reset the grid and never lend samples across segments. Frame authorization
continues to gate output positions: the immutable next block may supply a right
interpolation neighbor without advancing its converter position or playing its frames.
The SDL queue oracle flattens each segment and derives every sample position from
its absolute output index; MP3 expected source samples still come from FFmpeg.
Dummy SDL verifies submitted bytes, not physical speaker quality or device latency.


The v0.805 C89 file-export tutorial selects continuous interpolation. A dedicated
zero-allocation regression crosses 32 empty blocks after runtime teardown, checks
midpoint/final values and retry atomicity, and seeks directly to the final block.
Four OS workers independently exercise continuous converter cursors over shared
immutable storage. These workers do not establish race-free host destruction.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_converter_init` | function | [include/flashvm/swf_stream_convert.h:48](../include/flashvm/swf_stream_convert.h#L48) |
| `fvm_swf_stream_converter_init_continuous` | function | [include/flashvm/swf_stream_convert.h:56](../include/flashvm/swf_stream_convert.h#L56) |
| `fvm_swf_stream_converter_next` | function | [include/flashvm/swf_stream_convert.h:67](../include/flashvm/swf_stream_convert.h#L67) |
| `fvm_swf_stream_converter_seek` | function | [include/flashvm/swf_stream_convert.h:73](../include/flashvm/swf_stream_convert.h#L73) |
<!-- source-links:end -->
