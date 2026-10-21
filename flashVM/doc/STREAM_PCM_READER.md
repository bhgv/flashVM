# Prepared sequential stream PCM reader

[Русский](STREAM_PCM_READER.ru.md), [stream inspection](SWF_STREAM_SOUND.md).
`swf_stream_pcm.h` adds an allocation-free C89 cursor over an existing immutable
[FVM_SWFStreamSoundSnapshot](../src/swf/sound_stream.cpp#L18). It is a native preparation primitive for player audio
workers and file/channel adapters, not a real-time stream playback implementation.

## Ownership and output

Prepare the snapshot on the VM owner through [`fvm_swf_stream_sound_snapshot_open`](../include/flashvm/swf_stream_sound.h#L65).
Then initialize a caller-owned [FVM_SWFStreamPCMReader](../include/flashvm/swf_stream_pcm.h#L27). The snapshot owns copied
encoded bytes/index; the reader borrows it and allocates nothing. The original
context/runtime may be destroyed. Keep the snapshot alive until all readers are
quiesced, then close it exactly once. Readers need no destructor. Serialize each
cursor; separate readers may share the immutable snapshot. Disjoint descriptors,
PCM buffers and output counters are required. No VM pointer/root, callback, lock,
clock or mutable global is retained.

Each next call returns at most max_frames (>0), without crossing an encoded block.
Capacity and written count scalar native-S16 channel values; chunk.frames counts
sample frames (stereo pairs). Chunk metadata copies stream/block index, one-based
movie frame, block-relative first sample, source rate/channels and boundary flags:
BEGIN, END, EMPTY. These preserve variable block sizes and frame gaps. An explicit
empty block returns OK, zero values and BEGIN|END|EMPTY, advancing one block.
An absent movie-frame block emits nothing and never synthesizes silence.
EOF returns NOT_FOUND, preserving all outputs and the cursor.

This reader preserves actual source PCM geometry; header playback advice is not
resampling. PCM_NATIVE and PCM_LE use existing signed-S16 decoding; compressed
MP3/ADPCM stream segments return UNSUPPORTED at initialization. No codec, device,
mixer, seek crossfade, audio clock or A/V synchronization is introduced. Unmuted
root stream playback in the desktop player remains unsupported.

## API contracts

| API | Contract |
| --- | --- |
| [`fvm_swf_stream_pcm_reader_init`](../include/flashvm/swf_stream_pcm.h#L40) | Borrow a prepared PCM snapshot and copy header/count into native state at position 0/0. INVALID_ARGUMENT for null inputs, UNSUPPORTED for codec/short representation. No allocation; errors preserve the reader. |
| [`fvm_swf_stream_pcm_reader_next`](../include/flashvm/swf_stream_pcm.h#L48) | Read one bounded chunk and publish metadata/written only on OK. Requires exact chunk.struct_size, positive max_frames, enough scalar capacity and a PCM pointer for nonempty output. INVALID_ARGUMENT for descriptor/capacity/range errors; NOT_FOUND at EOF. Every error preserves cursor, PCM, metadata and written. Successful calls always advance even for empty/end markers. |
| [`fvm_swf_stream_pcm_reader_seek`](../include/flashvm/swf_stream_pcm.h#L54) | Set an indexed block and sample offset <= actual sample_count. block_count/0 is explicit EOF. End offsets yield a zero-value END marker on next; EMPTY is added only for an actually empty block. Invalid positions preserve the cursor. No decoding, allocation or output side effects. |

The reader has no self-reference and may be copied as a native position snapshot,
while retaining the same source-lifetime requirement. Seeking changes only this
cursor; already queued or consumed host PCM is not cancelled or rewound.

## Build and validation

The module is part of the core SDK and needs neither SDL nor an audio device:

```sh
cmake -S flashVM -B build-stream-reader -DFVM_BUILD_TESTS=ON
cmake --build build-stream-reader --target flashvm_tests_swf_stream_pcm_reader \
  flashvm_tests_swf_stream_pcm_reader_api flashvm_tests_swf_stream_sound_snapshot \
  flashvm_tests_swf_stream_sound_snapshot_allocation
ctest --test-dir build-stream-reader -R 'swf_stream_(pcm_reader|sound_snapshot)' --output-on-failure
```

Raw SWF fixtures exercise versions 3–8, native/little-endian PCM, mono/stereo,
8/16 bits, integer reference values, bounded chunks, empty blocks, frame gaps,
seek/end/EOF and atomic capacity failures. Snapshot allocation tests prohibit
new allocation during init/read/seek after original context/runtime destruction.
This evidence does not complete sound-device/reference-player/platform acceptance.

## Executable native cursor export (v0.750)

The [annotated swf_stream tutorial](../examples/multimedia/swf_stream/README.md)
adds `OUTPUT.pcm --cursor`. It prepares the same linked SWF stream, destroys its
context and runtime, then uses only the native reader and direct C file callbacks.
It does not recreate an output VM/channel in this mode. Each empty block is
reported but causes no write; chunks use actual source geometry. The file and
metadata report match both the live and older `--prepared` export paths exactly.
Missing output directories fail through ordinary cleanup, with no hidden device.

The native OS-port test starts four actual workers after VM/runtime destruction.
Each owns a reader and performs 500 rewind/read/EOF passes over the same immutable
snapshot, comparing every scalar value. Main joins all workers before closing
source storage. This is Linux native thread evidence; it does not certify foreign
OS concurrency, audible output, a real-time audio callback or A/V synchronization.

Validation at v0.750: 72 selected native regression checks and 14 shared-host checks pass. Reader/snapshot paths and the real file tutorial pass ASan/UBSan with an independent FFmpeg PCM oracle; leak detection is disabled by host restrictions and the whole VM is not instrumented. New implementation and C89 consumer/tutorial objects compile for ARM64 Linux/musl, RISC-V64 Linux/musl and Windows x64/MinGW, without claiming foreign execution or a complete cross-built player. The native OS-port worker target requires OS_PORTS enabled. All nine roadmap stages remain open.
[Explicit rate/channel conversion / Преобразование частоты/каналов](STREAM_PCM_CONVERSION.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_pcm_reader_init` | function | [include/flashvm/swf_stream_pcm.h:40](../include/flashvm/swf_stream_pcm.h#L40) |
| `fvm_swf_stream_pcm_reader_next` | function | [include/flashvm/swf_stream_pcm.h:48](../include/flashvm/swf_stream_pcm.h#L48) |
| `fvm_swf_stream_pcm_reader_seek` | function | [include/flashvm/swf_stream_pcm.h:54](../include/flashvm/swf_stream_pcm.h#L54) |
| `fvm_swf_stream_sound_snapshot_open` | function | [include/flashvm/swf_stream_sound.h:65](../include/flashvm/swf_stream_sound.h#L65) |
| `FVM_SWFStreamPCMReader` | struct | [include/flashvm/swf_stream_pcm.h:27](../include/flashvm/swf_stream_pcm.h#L27) |
| `FVM_SWFStreamSoundSnapshot` | struct | [src/swf/sound_stream_internal.hpp:13](../src/swf/sound_stream_internal.hpp#L13) |
<!-- source-links:end -->
