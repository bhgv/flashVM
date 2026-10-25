# Explicit prepared-stream memory budgets

[Русский](STREAM_STORAGE.ru.md) · [ADPCM preparation](STREAM_ADPCM.md) · [C89 tutorial](../examples/multimedia/swf_stream/README.md)

`fvm_swf_stream_sound_snapshot_open_pcm_bounded(context, stream, budget_bytes, out)`
adds a caller-selected **peak ownership** ceiling to native PCM preparation. It
accepts the same PCM and whole-packet ADPCM subset as
[`fvm_swf_stream_sound_snapshot_open_pcm`](../include/flashvm/swf_stream_sound.h#L110); no codec support changes. The context
owner calls preparation before publishing immutable native storage to workers.
The result is cleared first. Null output/context or invalid source arguments
follow the existing invalid/missing-source statuses; a valid zero-byte budget
returns OUT_OF_MEMORY because even an empty snapshot needs an ownership object.
Unsupported/malformed codecs retain their existing statuses. Any failed allocation
returns OUT_OF_MEMORY and releases temporary ownership. No device, callback or
movie/VM execution occurs.

The explicit ceiling includes the snapshot object, exact block-index allocation,
encoded-byte array and simultaneously allocated native-S16 array. The initial
object/index/encoded charge is checked **before** these allocations. Decoded
expansion is checked before allocating the PCM array or running its decoder.
The existing runtime bound on payload/index bytes applies independently; the
new function never raises or mutates that runtime configuration. Existing raw and
unbounded-normalization entry points retain their original payload-budget policy.
They also record storage charges but do not acquire a new caller ceiling.

`fvm_swf_stream_sound_snapshot_storage_info(snapshot, out)` copies
[`FVM_SWFStreamSoundStorageInfo`](../include/flashvm/swf_stream_sound.h#L115):

| Field | Meaning |
| --- | --- |
| `struct_size` | Caller supplies exact native `sizeof`; preserved on invalid input |
| `retained_bytes` | Snapshot object plus live block index and retained encoded or PCM arrays |
| `peak_bytes` | Maximum simultaneous object/index/encoded/decoded allocation charge during preparation |

For raw/unchanged PCM snapshots the two charges are equal. After ADPCM expansion,
encoded storage is released before return: peak minus retained is the encoded
payload size. Charges include empty snapshot/index ownership and are native
architecture-dependent values, not a portable wire schema. The observation is
allocation-free and requires no callback, device or original VM. It remains valid
after context/runtime teardown. Null snapshot/output or wrong `struct_size`
returns INVALID_ARGUMENT and preserves output. Multiple const readers can observe
a safely published snapshot; synchronize close and retain ownership until return.
Use [`fvm_swf_stream_sound_snapshot_close`](../include/flashvm/swf_stream_sound.h#L248) exactly once after all readers stop.

This is a deterministic charge for requested owned allocation sizes, **not RSS**.
Allocator bookkeeping/alignment overhead, stack objects, original VM SWF bytes,
managed heap, the host's segment table, other snapshots, retry buffers and device
memory are excluded. The implementation allocates exact initial array extents;
no speculative reserve or growing worker cache is hidden behind the observation.

## Encoded copies without decoding (v0.768)

`fvm_swf_stream_sound_snapshot_open_bounded(context, stream, budget_bytes, out)`
provides the same explicit ownership ceiling for the original encoded snapshot.
Use it for an external decoder, archival, diagnostics or transport of blocks. It
preserves the original codec identifier, payload bytes, header and frame metadata;
it does not accept a compressed stream as playable PCM. In particular MP3 prefix
views remain available after VM teardown, without adding an MP3 decoder.

Call on the context owner before publication. Supply a valid loaded context,
segment index, native byte budget and nonnull output pointer. The output is cleared
before validation. Source scanning retains the legacy raw API statuses; it validates
SWF structure and PCM alignment, not compressed codec contents. For example a short
MP3 block may be copied successfully and then rejected by the MP3-prefix view.
An insufficient/zero budget returns OUT_OF_MEMORY before ownership allocation;
allocation failures release every temporary allocation. Null arguments return
INVALID_ARGUMENT; absent indices keep the raw API's missing-source status.
No allocation, callback, decoder, device operation or VM execution occurs on later
immutable queries. Opening itself allocates and copies; it never calls user code.
The runtime payload limit remains independent and unchanged. On successful raw
preparation, retained and peak charges are equal. Use ordinary snapshot_close after
all readers finish, including when the source context/runtime have been destroyed.
Concurrent reads require safe publication and externally synchronized destruction.

The C89 tutorial now explicitly uses the encoded bounded entry point for its PCM
fixture, illustrating that unchanged PCM does not require the normalization path.
ADPCM playback continues to use open_pcm_bounded so worker input remains native S16.

## Bound a group of snapshots

Start with an explicit group budget and subtract any separately owned segment
table/buffers first. For each source in acquisition order, pass the remaining
budget to the bounded preparation API. On success query its charges and subtract
**retained**, not peak, before preparing the next source. Keep earlier snapshots
alive during actual acquisition. This guarantees the charged preparation peak
of each new snapshot plus retained predecessors fits the group ceiling. If later
preparation fails, close all earlier snapshots before releasing their owner table.
The operation is not a transactional rollback of unrelated host/VM effects.

A preflight can perform the same arithmetic while immediately closing each temporary
snapshot, retaining the charges virtually. That duplicates copying/decoding during
startup but needs no hidden retained cache and predicts the same acquisition budget.
It is not an aggregate limit on unrelated event voices, windows or all process memory.

## Executable example and checks

The C89 [stream tutorial](../examples/multimedia/swf_stream/README.md) uses a one-MiB
explicit peak limit in prepared modes. `OUTPUT.pcm --storage` destroys the input
VM, queries storage charges, prints a native-size report and exports the same exact
PCM through its independent cursor. No audio device or third-party decoder is needed.

```sh
cmake --build build --target flashvm_examples_multimedia_swf_stream_main flashvm_tests_swf_stream_storage
./build/flashvm_examples_multimedia_swf_stream_main output.pcm --storage
./build/flashvm_tests_swf_stream_storage
python3 scripts/check_swf_stream_example.py build/flashvm_examples_multimedia_swf_stream_main
```

`swf_stream_storage.cpp` verifies exact/one-byte-short/zero ceilings, an independent
runtime payload limit, raw versus normalized charges, unchanged PCM, invalid-output
preservation and observation/reads after VM teardown. The allocation-failure fixture
uses the bounded path and queries storage with allocation allowance zero after
teardown. Public C89 header/link checks include the observation and both bounded APIs. Native charge checks do
not certify allocator RSS, audible devices or execution on untested platforms.
Revision checks: all 21 selected native core stream/ADPCM tests and the C89 tutorial
checks passed, including exact FFmpeg output. Edited stream modules and storage/
ADPCM fixtures passed ASan/UBSan; allocation budgets 0–4 used isolated processes.
Leak enumeration is unavailable on this restricted host, and the full VM/dependency
tree was not sanitizer-instrumented.
Revision v0.768 extends raw boundary and allocation-failure checks, and MP3 byte/
prefix preservation across source replacement. Earlier revision results above
remain historical; see the delivered revision report for rerun scope.


Revision v0.768 rerun: 21 native core stream/ADPCM tests, exact C89 tutorial and
FFmpeg oracles passed. Edited modules passed ASan/UBSan, with raw failure budgets
0–3 and PCM budgets 0–4 isolated per process. This host cannot enumerate leaks;
the complete VM and SDL were not instrumented.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:248](../include/flashvm/swf_stream_sound.h#L248) |
| `fvm_swf_stream_sound_snapshot_open_bounded` | function | [include/flashvm/swf_stream_sound.h:92](../include/flashvm/swf_stream_sound.h#L92) |
| `fvm_swf_stream_sound_snapshot_open_pcm` | function | [include/flashvm/swf_stream_sound.h:110](../include/flashvm/swf_stream_sound.h#L110) |
| `fvm_swf_stream_sound_snapshot_open_pcm_bounded` | function | [include/flashvm/swf_stream_sound.h:128](../include/flashvm/swf_stream_sound.h#L128) |
| `fvm_swf_stream_sound_snapshot_storage_info` | function | [include/flashvm/swf_stream_sound.h:153](../include/flashvm/swf_stream_sound.h#L153) |
| `FVM_SWFStreamSoundStorageInfo` | struct | [include/flashvm/swf_stream_sound.h:115](../include/flashvm/swf_stream_sound.h#L115) |
<!-- source-links:end -->
