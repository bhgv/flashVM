# Inferred partial ADPCM preparation

`fvm_swf_stream_sound_snapshot_open_pcm_inferred(context, stream, budget_bytes, out)`
adds an explicit bounded preparation policy for root stream ADPCM. The desktop
player now uses it before acquiring the native device or publishing its worker.
The original open_pcm/open_pcm_bounded APIs keep their whole-packet-only behavior;
the counted API still handles externally known lengths, including ambiguity.

## Length policy

First preserve the legacy canonical whole-4096-frame packet interpretation. When
that policy returns UNSUPPORTED, inspect every possible final zero-padding length
from zero to seven bits. Remove the two code-width bits, account for full packets,
then require a final predictor/index frame and integral interleaved differential
codes. There is no byte alignment between packets. Exactly one endpoint is
accepted; multiple endpoints return UNSUPPORTED; none returns INVALID_ARGUMENT.
The decoder then validates and decodes the complete chosen stream.

This preserves compatibility for canonical whole packets even where another
shorter geometry could fit their bytes. The new partial fallback never chooses
by header average, movie FPS, block ordinal, nearest count or longest candidate.
An empty block/valid width-only empty payload keeps the legacy zero-frame policy.
Trailing zero differential codes are data when the unique geometry includes them;
trimming all zero bits would lose real samples. Length geometry cannot establish
an author's intention or detect every truncation that itself forms a valid block.

## API ownership, status and budget

Use a live context on its owner thread, zero-based root stream index, explicit
peak-byte budget and required disjoint writable snapshot pointer. Out is cleared
before other validation. Borrowed source bytes/context must stay stable during
synchronous preparation. Success returns independently owned immutable normalized
native S16 PCM, preserving source rate/channels, frame/block identities, playback
metadata and advisory average. Original source bytes remain encoded ADPCM.
Other supported PCM/MP3 formats follow open_pcm_bounded policy unchanged.

Snapshot object, index, encoded bytes and decoded PCM peak share the ceiling;
subtract each predecessor's retained bytes for aggregate segment ownership. Zero
budget is insufficient even for an empty owner. Arithmetic overflow, native
allocation failure, runtime payload/index limit or peak excess returns
OUT_OF_MEMORY. Invalid pointers/source/unsupported representation and missing
stream statuses follow the existing bounded preparation contract. Bit geometry
errors and ambiguous partial statuses are described above. Failure releases all
temporary owners. No partial snapshot is published and no host audio endpoint
is opened by this API.

Preparation allocates and decodes; do it before worker publication, outside audio
callbacks. It performs no script/device callback or VM execution and retains no
managed/GC pointer. The source VM may be destroyed on return. Readers/converters
borrow the snapshot; independent cursors may share immutable data on native OS
workers. Externally synchronize source mutation and close; close only after all
readers/workers stop. This does not add locks, worker creation or shared cursors.

## Desktop and native consumers

Desktop prepare_segment uses the inferred policy under its existing retained/peak
aggregate budget before audio acquisition. Ambiguous/malformed audio still fails
preflight; explicit mute retains its existing policy. Both event-free stream and
root stream/event mixing use normalized PCM. There is no change to retry, frame
gates, finite tails, StopSounds, seek-clear epochs, source grids or dequeue clocks.
No automatic source-time/A-V policy is introduced.

The C89 swf_stream_adpcm example adds --inferred without consulting block_frames.
Its final nonzero codes establish five and three frames around an empty block;
all three export modes produce the same eight hand-calculated values after VM
teardown. See the [annotated example](../examples/multimedia/swf_stream_adpcm/README.md).

## Evidence and open acceptance

The 104-case matrix spans widths 2–5, mono/stereo and counts around 4096/8192:
65 accepted cases match every sample; 39 ambiguous cases remain rejected. The
oracle enumerates candidate counts independently rather than copying endpoint
arithmetic. Native tests cover malformed endings/short predictors, exact and
one-byte-short budgets, all four allocation acquisitions, allocation-free reads,
source teardown, exact PCM retry, held frame gate, failed clear and acknowledged
seek reset. Four real OS workers use one immutable snapshot and independent
converters across 128 epochs, comparing rational interpolation at packet restart.

Real SDL dummy CLI fixtures compare all accepted PCM for FWS/CWS, interpreter
and native JIT, empty blocks, a partial restart and a PCM successor. Mixed fixtures
check partial stereo ADPCM plus opposing events, one saturation, exact BASE19 and
TAIL0 under both finite policies. These are Linux native functional checks, not
physical audio/DAC, a race-detector claim or foreign-platform acceptance.

Ambiguous ADPCM still needs authoritative counts. Child timelines/dictionaries,
reference codec latency/seek, source timestamps, automatic A/V drift and complete
SWF <=8 support remain open. All nine roadmap stages remain open.


v0.1064: Add bounded PCM preparation that preserves canonical whole packets and accepts a unique zero-padded partial length without header averages.

v0.1065: Enumerate independent frame-count candidates for 104 geometries, checking every decoded sample and preserving ambiguous rejection and legacy whole packets.

v0.1066: Exercise all four native acquisitions, exact and short budgets, VM teardown and allocation-free immutable PCM reads.

v0.1067: Check invalid endings and short predictors, ambiguous mono rejection, missing owners and a unique nonzero endpoint with exact PCM.

v0.1068: Use the inferred bounded snapshot policy for desktop stream segments while retaining aggregate peak budgets and preflight failure before device ownership.

v0.1069: Check FWS CWS interpreter JIT exact rational PCM, packet restart, empty blocks, PCM successor and rejection before device acquisition.

v0.1070: Real workers verify one-saturation stereo PCM and BASE19 TAIL0 for both finite policies, compressed SWF and both execution modes.

v0.1071: Add an inferred export policy without count-table access; verify the same eight hand-calculated samples in all three ownership modes.

v0.1072: Four native workers share a post-VM snapshot with independent cursors, checking exact rational interpolation and packet restart over 128 epochs.

v0.1073: Post-VM converted PCM survives rejected writes, a closed frame gate, failed clear and acknowledged reset; short final retry drains exactly 19 stereo frames.

v0.1074: Specify canonical compatibility, unique partial endpoints, unchanged count-authoritative APIs, peak budgets and native versus physical acceptance in Russian and English.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_snapshot_open_pcm_inferred` | function | [include/flashvm/swf_stream_sound.h:136](../include/flashvm/swf_stream_sound.h#L136) |
<!-- source-links:end -->
