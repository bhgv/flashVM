# Prepared whole-packet ADPCM root streams

Explicit host-authoritative partial lengths: [counted ADPCM](STREAM_ADPCM_COUNTED.md). The automatic path below retains whole-packet-only inference.

[Русский](STREAM_ADPCM.ru.md) · [PCM snapshots](SWF_STREAM_SOUND.md) · [player](PLAYER_PCM_STREAMS.md)

[`fvm_swf_stream_sound_snapshot_open_pcm`](../include/flashvm/swf_stream_sound.h#L87) is an additive C89 preparation API.
Call on the owning context before native worker publication. The caller owns the
returned immutable snapshot and closes it with [`fvm_swf_stream_sound_snapshot_close`](../include/flashvm/swf_stream_sound.h#L160).
Its data remains readable after context/runtime teardown, without VM calls,
allocation, codec work or callbacks in the PCM reader/converter/output worker.
It does not advance MovieClip, open devices or schedule audio.

PCM_NATIVE/PCM_LE input follows the existing raw snapshot path unchanged.
ADPCM preparation accepts complete 4096-frame packets with 2-, 3-, 4- or 5-bit
codes, mono/stereo, independent width/predictor/index state in each SoundStreamBlock.
The two-bit width prefix precedes all packets in that block. At most seven final
zero bits are padding. A zero-byte block or width-only byte with zero padding is
an empty marker. Multiple complete packets in one block are allowed. No inferred
sample count comes from the header's advisory average.

This explicit subset follows Adobe's packet subdivision model, described in the
[SWF specification, streaming sound and ADPCM sections](https://open-flash.github.io/mirrors/swf-spec-19.pdf#page=185).
Partial packets lack an explicit sample count in SoundStreamBlock; this preparation
returns UNSUPPORTED rather than guessing a shorter final count. This is a bounded
implementation policy, not a claim that every historical Flash file uses complete
packets or that every possible stream is supported.

On success ADPCM becomes PCM_NATIVE, 16-bit, at the original rate/channel count.
Stream/block indices, frame numbers, advisory playback fields and average remain;
block `sample_count_known` becomes true and sizes/counts describe decoded PCM.
[`fvm_swf_stream_sound_snapshot_info`](../include/flashvm/swf_stream_sound.h#L127) and [`fvm_swf_stream_sound_snapshot_block_at`](../include/flashvm/swf_stream_sound.h#L131)
therefore describe the **normalized** storage. The original live APIs and
[`fvm_swf_stream_sound_snapshot_open`](../include/flashvm/swf_stream_sound.h#L65) still expose unchanged opaque ADPCM bytes.
Choose the raw API if original compressed bytes are needed. No public structure
layout changes. Existing readers/converters consume normalized data as ordinary PCM.

Arguments: context and selected zero-based stream index; output snapshot pointer
is required and cleared first. Invalid context/index statuses propagate from the
raw snapshot path. Other codecs/partial packets return UNSUPPORTED; nonzero final
padding returns INVALID_ARGUMENT. Allocation failure and preparation budget excess
return OUT_OF_MEMORY. The raw encoded payload, decoded S16 payload and exact block
index must together fit the configured runtime memory-limit value (or existing
SWF default). This is a per-preparation payload/index bound, not aggregate process
RSS accounting across every live snapshot. Compressed storage is released before
successful return. Temporary ownership is released on failure; no partial snapshot
is published. Never mutate or close a snapshot while a reader uses it.

The existing [C89 stream tutorial](../examples/multimedia/swf_stream/README.md)
uses this API in prepared modes with its original PCM fixture. For independent
compressed validation build `flashvm_tests_swf_stream_adpcm`, then run:

```sh
./build/flashvm_tests_swf_stream_adpcm
python3 scripts/check_stream_adpcm_ffmpeg.py build/flashvm_tests_swf_stream_adpcm
```

FFmpeg is an optional **test oracle**, not a runtime dependency. The Python test
packs varying differential codes independently into one-/two-packet root blocks,
adds zero-byte/width-only markers and a deliberately inaccurate average, and
compares all decoded samples for eight width/channel combinations in FWS and CWS.
Its exporter destroys the source VM before reading. The C++ fixture also checks
raw-view preservation, budget rejection, nonzero padding, truncated packet refusal,
packet restart values and cursor chunk boundaries. These checks do not certify
physical audio output, Flash drift/seek behavior, MP3/Nellymoser or foreign CPU execution.
Revision checks: 20 native core stream/ADPCM tests passed. Edited stream modules
and both new fixtures passed ASan/UBSan, with allocation budgets 0–4 isolated
in separate processes. LeakSanitizer cannot enumerate processes on this restricted
host and was disabled; the entire VM/dependency tree was not sanitizer-instrumented.
The broader multimedia comment checker still reports pre-existing missing comments
in unchanged Windows speech files; it is not reported as passing here.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_snapshot_block_at` | function | [include/flashvm/swf_stream_sound.h:131](../include/flashvm/swf_stream_sound.h#L131) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:160](../include/flashvm/swf_stream_sound.h#L160) |
| `fvm_swf_stream_sound_snapshot_info` | function | [include/flashvm/swf_stream_sound.h:127](../include/flashvm/swf_stream_sound.h#L127) |
| `fvm_swf_stream_sound_snapshot_open` | function | [include/flashvm/swf_stream_sound.h:65](../include/flashvm/swf_stream_sound.h#L65) |
| `fvm_swf_stream_sound_snapshot_open_pcm` | function | [include/flashvm/swf_stream_sound.h:87](../include/flashvm/swf_stream_sound.h#L87) |
<!-- source-links:end -->
