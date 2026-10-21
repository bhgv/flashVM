# Partial ADPCM blocks with authoritative counts (v0.773)

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
[fvm_swf_stream_sound_snapshot_close](../include/flashvm/swf_stream_sound.h#L131) after every reader/converter has stopped.

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
[fvm_swf_stream_sound_snapshot_storage_info](../include/flashvm/swf_stream_sound.h#L117) exposes exact retained/peak charges,
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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:131](../include/flashvm/swf_stream_sound.h#L131) |
| `fvm_swf_stream_sound_snapshot_open_adpcm_counted` | function | [include/flashvm/swf_stream_sound.h:111](../include/flashvm/swf_stream_sound.h#L111) |
| `fvm_swf_stream_sound_snapshot_storage_info` | function | [include/flashvm/swf_stream_sound.h:117](../include/flashvm/swf_stream_sound.h#L117) |
<!-- source-links:end -->
