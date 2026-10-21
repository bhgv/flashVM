# Explicit prepared-stream memory budgets

[Русский](STREAM_STORAGE.ru.md) · [ADPCM preparation](STREAM_ADPCM.md) · [C89 tutorial](../examples/multimedia/swf_stream/README.md)

`fvm_swf_stream_sound_snapshot_open_pcm_bounded(context, stream, budget_bytes, out)`
adds a caller-selected **peak ownership** ceiling to native PCM preparation. It
accepts the same PCM and whole-packet ADPCM subset as
[`fvm_swf_stream_sound_snapshot_open_pcm`](../include/flashvm/swf_stream_sound.h#L73); no codec support changes. The context
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
[`FVM_SWFStreamSoundStorageInfo`](../include/flashvm/swf_stream_sound.h#L77):

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
Use [`fvm_swf_stream_sound_snapshot_close`](../include/flashvm/swf_stream_sound.h#L107) exactly once after all readers stop.

This is a deterministic charge for requested owned allocation sizes, **not RSS**.
Allocator bookkeeping/alignment overhead, stack objects, original VM SWF bytes,
managed heap, the host's segment table, other snapshots, retry buffers and device
memory are excluded. The implementation allocates exact initial array extents;
no speculative reserve or growing worker cache is hidden behind the observation.

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
teardown. Public C89 header/link checks include both APIs. Native charge checks do
not certify allocator RSS, audible devices or execution on untested platforms.
Revision checks: all 21 selected native core stream/ADPCM tests and the C89 tutorial
checks passed, including exact FFmpeg output. Edited stream modules and storage/
ADPCM fixtures passed ASan/UBSan; allocation budgets 0–4 used isolated processes.
Leak enumeration is unavailable on this restricted host, and the full VM/dependency
tree was not sanitizer-instrumented.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:107](../include/flashvm/swf_stream_sound.h#L107) |
| `fvm_swf_stream_sound_snapshot_open_pcm` | function | [include/flashvm/swf_stream_sound.h:73](../include/flashvm/swf_stream_sound.h#L73) |
| `fvm_swf_stream_sound_snapshot_open_pcm_bounded` | function | [include/flashvm/swf_stream_sound.h:88](../include/flashvm/swf_stream_sound.h#L88) |
| `fvm_swf_stream_sound_snapshot_storage_info` | function | [include/flashvm/swf_stream_sound.h:93](../include/flashvm/swf_stream_sound.h#L93) |
| `FVM_SWFStreamSoundStorageInfo` | struct | [include/flashvm/swf_stream_sound.h:77](../include/flashvm/swf_stream_sound.h#L77) |
<!-- source-links:end -->
