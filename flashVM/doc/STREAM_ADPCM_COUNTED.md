# Partial ADPCM blocks with authoritative counts (v0.773)

C89 prepared/range export tutorial (v0.775): [annotated example](../examples/multimedia/swf_stream_adpcm/README.md). The fixture checks eight hand-calculated samples, an empty block and native file errors after VM teardown.

[Русский](STREAM_ADPCM_COUNTED.ru.md) · [automatic whole packets](STREAM_ADPCM.md) · [storage budgets](STREAM_STORAGE.md)

`fvm_swf_stream_sound_snapshot_open_adpcm_counted(context, stream, counts, count, budget_bytes, out)`
prepares independent native S16 PCM from root ADPCM blocks, including partial final
4096-frame packets. A host that knows the exact source counts can use the ordinary
native reader, converter and stream output without retaining the source VM.
This is an explicit native decoding path, not automatic desktop partial-stream playback.

## Why counts are explicit

SoundStreamHead's average_samples is advisory. Short ADPCM codes and zero byte
padding can describe several possible final frame counts. For example mono two-bit
codes permit both two and three frames in the same four-byte payload. Neither a
movie's frame rate nor its average is permission to choose one. The caller supplies
an authoritative count from its asset pipeline or another independently established
contract. The API verifies that the assertion fits the bytes, not that it equals
the original author's intention. Existing open_pcm/open_pcm_bounded keep their
whole-packet-only inference; no decoder silently reinterprets their input.

## Arguments, statuses and ownership

| Argument | Contract |
| --- | --- |
| context | Live source context, used synchronously on its owner thread; no script execution |
| stream | Zero-based root SoundStreamHead/Head2 occurrence |
| counts | Borrowed initialized size_t array, one source-frame count per encoded block; never retained |
| count | Exact number of entries, including empty blocks; null counts allowed only for zero entries |
| budget_bytes | Peak native snapshot ceiling including object, index, encoded and decoded payloads |
| out | Required writable pointer, disjoint from input storage; cleared before validation |

Success returns OK and publishes an immutable normalized PCM_NATIVE/S16 snapshot.
Rate, channels, root frame/block identities, advisory playback and average fields
remain unchanged. The original live movie and encoded snapshot APIs remain opaque
ADPCM; source bytes are not rewritten. The count array can be released immediately
on return. The snapshot can outlive context/runtime, then closes through
[fvm_swf_stream_sound_snapshot_close](../include/flashvm/swf_stream_sound.h#L248) after every reader/converter has stopped.

INVALID_ARGUMENT covers null/wrong arguments, table length mismatch, counts beyond
ULONG_MAX, truncated/excess encoded bytes and nonzero padding. Source validation
and missing stream statuses propagate. Non-ADPCM input returns UNSUPPORTED when
the count table length matches; a length mismatch takes precedence. OUT_OF_MEMORY
covers native arithmetic overflow, allocations, runtime payload/index limit and
explicit peak ceiling; zero is a valid insufficient budget. Failures free every
temporary and leave no snapshot owner. Backend/device callbacks, VM execution and
thread creation never occur. Preparation allocates and decodes: do it before worker
publication, not on a real-time audio callback. Serialize source changes/teardown;
immutable native reads may run concurrently with separate reader cursors, but close
requires quiescence. No managed pointer or GC root is retained.

The same storage accounting as bounded open_pcm applies. Encoded plus decoded
payloads coexist during preparation; compressed bytes are freed on success.
[fvm_swf_stream_sound_snapshot_storage_info](../include/flashvm/swf_stream_sound.h#L153) exposes exact retained/peak charges,
excluding allocator bookkeeping, VM, count table, stack and caller reader buffers.
Subtract retained charges when preparing multiple independently owned segments;
this API does not enforce a process-wide aggregate memory limit.

## Bit geometry and empty blocks

Every nonempty encoded block starts with its own two-bit code-width field.
Packets contain one predictor/index per channel and up to 4095 differential frames;
next-packet predictors follow immediately without byte alignment. A final partial
packet is legal for every width 2–5 and mono/stereo. The exact declared bit geometry
must occupy all payload bytes except at most seven final **zero** padding bits.
One count does not describe an entire segment: each block restarts its decoder.
Zero frames accepts a zero-byte block or a width-only padded byte. A zero-length
entry still occupies its ordinal in the count table; no movie gap inserts silence.

## Reproducible checks

```sh
cmake --build build --target flashvm_tests_swf_stream_adpcm_counted \
  flashvm_tests_swf_stream_adpcm_counted_allocation
ctest --test-dir build --output-on-failure -R 'swf_stream_adpcm|swf_stream_storage'
python3 scripts/check_counted_adpcm_ffmpeg.py build/flashvm_tests_swf_stream_adpcm_counted
```

The native matrix checks all widths/channels at counts 0, 1, 2, 3, 7, 17, 4095,
4096, 4097, 8191, 8192 and 8193: every decoded sample, reader EOF after VM teardown,
exact peak/one-byte-short budgets, malformed padding/lengths and explicit ambiguity.
The allocation fixture injects failures at all four ownership acquisitions and
checks allocation-free post-VM native reads. Its optional numeric argument selects
one allocation budget 0–4 for an isolated sanitizer process.

The optional Python oracle builds independent varying-code FWS/CWS movies and
compares complete selected PCM against FFmpeg for every width/channel combination,
including partial restart tails, empty blocks and a deliberately false average.
FFmpeg is a test executable only. Physical audio, full desktop mixed playback,
MP3, child streams, movie timing/A/V drift and foreign platforms remain separate
open acceptance work. All roadmap stages remain open.


## v0.773 validation boundary

All 57 native sound/stream regressions pass in the strict C89/C++98 Linux minimal
Release build. The counted matrix passes with the entire minimal core built using
ASan/UBSan. All five isolated allocation budgets pass with UBSan on the changed
stream module and fixture, linked to the ordinary Release core. Allocation success
also passes full-core ASan/UBSan. ASan fault-unwind runs are not accepted: this
restricted host cannot observe the stack through /proc and reports no-return/
stack-poisoning diagnostics after injected exceptions. LeakSanitizer is disabled
because process enumeration is unavailable. No physical/foreign acceptance claimed.


## Allocation-free ranges (v0.774)

[fvm_swf_stream_sound_read_adpcm](../include/flashvm/swf_stream_sound.h#L236)(context, stream, block, samples, first, frames,
out, capacity_values, written_values) decodes one live root ADPCM block on the
context owner. [fvm_swf_stream_sound_snapshot_read_adpcm](../include/flashvm/swf_stream_sound.h#L242)(snapshot, block, samples,
first, frames, out, capacity_values, written_values) reads an **encoded** immutable
snapshot, including after source VM teardown. Normalized PCM snapshots return
UNSUPPORTED; use the ordinary PCM reader for those.

Both use an authoritative total samples count for the selected block, followed
by a first/frames source-frame range. capacity_values and written_values count
interleaved S16 scalar channel values. Positive frames require aligned output and
sufficient capacity; zero frames permits null output. written_values is required
and cleared before source validation. Keep input/output/count storage disjoint.
No destination PCM changes on failure. The complete declared bit length, padding
and code stream are validated even for an early or zero-length range. Empty
payload and width-only zero-frame policies match counted preparation.

Null/wrong pointers, range overflow, insufficient capacity, nonzero padding or
length/count mismatch return INVALID_ARGUMENT; samples beyond ULONG_MAX also
returns INVALID_ARGUMENT. Non-ADPCM or unsupported native S16 representation
returns UNSUPPORTED. Bit arithmetic overflow returns OUT_OF_MEMORY although
neither function allocates. Source validation/missing stream/block statuses
propagate, with written count zero. No callback, VM execution, mutation, native
owner acquisition or managed/GC reference occurs. The live context requires owner
serialization; encoded snapshots permit concurrent immutable reads into separate
buffers, with close/source teardown synchronized separately.

Decoding validates from the beginning: runtime is linear in total block length,
not selected output length. Repeated tiny reads repeat that work. Use this path
for inspection, bounded extraction or native asset analysis; prepare normalized
PCM once for continuous low-latency playback. There is no automatic parallel
worker, desktop scheduling, child-stream integration or clock inference.

The range regression tests every width/channel geometry across packet restarts,
wrong capacity/ranges/codecs, invalid suffixes even for empty reads, source reload
and post-VM encoded reads. The C89 fixture invokes all three counted/range APIs
against literal SWF bytes. The allocation fixture proves both live and native
range reads need no allocator; the FFmpeg oracle checks the complete file again
through native range extraction after VM teardown.


### Range verification

All 60 native sound/stream checks pass in the Linux minimal Release build.
ASan/UBSan with the full minimal core passes the counted/range matrices, C89 ABI,
successful ownership path and four real native workers reading a shared encoded
snapshot after VM teardown. Changed stream code and allocation fixture pass all
five isolated UBSan allocation budgets, including allocation-free live/native reads.
The earlier ASan exception-unwind and LeakSanitizer host limitations still apply.
FFmpeg FWS/CWS comparison passes for both prepared and range exporters in all
eight width/channel combinations. These are Linux functional checks, not physical
sound, foreign-platform acceptance or a race-detector claim.

```sh
cmake --build build --target flashvm_tests_swf_stream_adpcm_ranges \
  flashvm_tests_swf_stream_adpcm_ranges_c89 flashvm_tests_swf_stream_adpcm_ranges_os_ports
ctest --test-dir build --output-on-failure -R 'swf_stream_adpcm'
```


## C89 tutorial verification (v0.775)

The annotated example passes ordinary strict C89 and full-minimal-core ASan/UBSan
in both prepared and encoded-range modes. All eight hand-calculated values, file
replacement, no-side-effect usage, malformed CLI and terminal file-open errors
pass the Python checker. The focused CTest run passes 62 checks: 60 native
sound/stream fixtures, the example usage and its complete export checker.
Source-link freshness, version refresh and sorted example catalog checks pass.
The global multimedia comment checker still reports only two unchanged Windows
speech files; changed tutorial files have complete line comments and API coverage.


Explicit inferred partial preparation and automatic desktop integration: [EN](INFERRED_ADPCM.md) / [RU](INFERRED_ADPCM.ru.md). Existing whole-only/count-authoritative APIs keep their policy. / Неоднозначные блоки по-прежнему требуют внешних длин; все стадии открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_read_adpcm` | function | [include/flashvm/swf_stream_sound.h:236](../include/flashvm/swf_stream_sound.h#L236) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:248](../include/flashvm/swf_stream_sound.h#L248) |
| `fvm_swf_stream_sound_snapshot_open_adpcm_counted` | function | [include/flashvm/swf_stream_sound.h:145](../include/flashvm/swf_stream_sound.h#L145) |
| `fvm_swf_stream_sound_snapshot_read_adpcm` | function | [include/flashvm/swf_stream_sound.h:242](../include/flashvm/swf_stream_sound.h#L242) |
| `fvm_swf_stream_sound_snapshot_storage_info` | function | [include/flashvm/swf_stream_sound.h:153](../include/flashvm/swf_stream_sound.h#L153) |
<!-- source-links:end -->
