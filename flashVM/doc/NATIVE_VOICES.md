# Bounded native event voices

## v0.783: exact storage and preparation ceilings

[`fvm_swf_sound_voice_open_bounded`](../include/flashvm/swf_sound.h#L97) retains the existing PCM/ADPCM/MP3, range,
loop and envelope behavior of [`fvm_swf_sound_voice_open`](../include/flashvm/swf_sound.h#L82). Its additional budget
covers the native object, exact selected-interval S16 array and copied envelope
records. Check addition/multiplication before allocating any of these owners;
even an empty voice needs its native object. Zero or insufficient budget returns
OUT_OF_MEMORY and null output. Runtime payload limits still apply independently.
Codec/source errors propagate. Decoder stack, VM allocations, allocator overhead,
process RSS and output queues are excluded. The ceiling bounds storage, not CPU.

[`fvm_swf_sound_voice_storage_info`](../include/flashvm/swf_sound.h#L101) copies exact retained/PCM/envelope charges
without allocating, callbacks or source access. Caller sets exact struct_size;
invalid arguments preserve output. The voice remains independently owned after
reload, GC and context teardown. All operations require external serialization;
source preparation runs on the context owner. Close with voice_close.

Tests: `swf_sound_voice_storage` checks exact and one-byte-short boundaries for
six independently generated MPEG geometries and mono/stereo PCM, output lifetime,
null/wrong-size arguments and unchanged query output. These focused checks do
not certify complete SWF sound, reference timing or physical device behavior.

## v0.784: reusable prepared PCM

[`fvm_swf_sound_voice_clone`](../include/flashvm/swf_sound.h#L111) makes a deep independent copy of a prepared voice,
including its current cursor, repetition and envelope segment, or EOF. It never
re-enters a context or decoder. The explicit budget includes the object and exact
arrays. On failure the source is unchanged and output is null. Keep source and
output ownership storage disjoint and serialize the source through return.
After success the two owners share nothing mutable; distinct workers may read
and close them independently. This deliberately copies PCM rather than adding a
shared cross-thread reference count. A host can keep a template after VM teardown
and clone it for overlap without repeating MP3/ADPCM decode. Memory is not free:
charge every template and clone. Close each untransferred result separately.

`swf_sound_voice_clone` compares original/copy after partial reads, repeats,
envelope changes and EOF, then destroys the original and continues the copy.
Exact/short budgets and argument preservation are exercised after source teardown.

## v0.785: native cursor positioning

[`fvm_swf_sound_voice_seek`](../include/flashvm/swf_sound.h#L123) selects a repeat and source-relative frame without
allocation or decoding. For a nonempty voice choose play_index < play_count and
frame < frames_per_play, or (play_count,0) for EOF. Empty/zero-play voices accept
only (0,0), remaining finished. Invalid input leaves all progress unchanged.
(0,0) restarts a nonempty template. The next read reconstructs envelope time from
the absolute elapsed repeats; backward seeks reset cached envelope lookup.
Serialize the caller-owned voice. Never seek an already transferred handle.
This positions prepared source PCM, not a movie timeline, conversion grid, mixer,
pending queue block or hardware clock. Those owners need their own reset policy.

`swf_sound_voice_seek` compares every position in a three-repeat, three-envelope
fixture with a previously rendered independent sequential result, in reverse
order after VM destruction. It checks restart, canonical EOF, invalid unchanged
progress, empty and zero-loop owners with allocation forbidden throughout seeks.

## v0.786: aggregate mixer ownership

[`fvm_swf_sound_mixer_storage_info`](../include/flashvm/swf_sound_mixer.h#L98) returns fixed mixer object/scratch/slot bytes,
all owned voice bytes and their checked sum. Release tails and converter lookahead
remain charged until the slot actually retires. Invalid query arguments preserve
output. No source VM, callback or allocation is involved.

[`fvm_swf_sound_mixer_submit_prepared_bounded`](../include/flashvm/swf_sound_mixer.h#L86) applies the existing transfer and
SyncNoMultiple/finished policy with an aggregate object/slot/voice ceiling.
Budget rejection leaves the incoming handle, progress and all old slots intact.
NoMultiple and finished no-ops consume their unused owner without adding storage,
even with a zero budget. Ordinary adoption includes old plus new voice objects,
PCM and envelope arrays. Queue buffers, allocator overhead and external templates
are excluded. Keep all owners serialized; no callback/VM/allocation occurs.

[`fvm_swf_sound_mixer_sound_active`](../include/flashvm/swf_sound_mixer.h#L104) queries the same non-releasing same-ID condition
used by SyncNoMultiple. Valid IDs are 1..65535; errors preserve output. An
observation is not a reservation: recheck controls when adopting under the lock.
`swf_sound_mixer_storage` forbids allocations after VM teardown and verifies exact
aggregate boundaries, rejected-owner preservation, no-op consumption, converted
voice retirement and restored fixed-only storage.

## v0.787: owner preparation without blocking the producer

The desktop StartSound callback now resolves event metadata on the context owner,
briefly locks to apply stops/SyncNoMultiple or observe remaining mixer bytes,
then unlocks before PCM/ADPCM/MP3 validation, allocation and decode. A second
short serialized phase transfers the prepared voice with an aggregate recheck.
Failed temporary owners are freed outside the lock. The native worker never
receives a context, clip, decoder or borrowed SWF data. Owner callbacks must not
run concurrently; during preparation only the worker can retire old voices, so
the observed remaining byte allowance is conservative. SyncNoMultiple checks
activity before expensive preparation and is rechecked on actual adoption.

FVM_DESKTOP_AUDIO_EVENT_BYTES defaults to 64 MiB, independently overridable from
the streaming preparation budget. It charges fixed mixer/slots plus all retained
and incoming voice owners. Mixer construction and its fixed-byte check occur
before SDL audio device open. VM, queue/history/trace buffers and allocator
bookkeeping remain outside this source-storage ceiling. A rejected event returns
its error and does not destroy earlier voices; no stealing or silent dropping.
This does not promise bounded owner-thread decode time or real-time playback.

The actual SDL `flashvm_frontend_audio_event_budget` fixture uses a 64 KiB build:
a first voice survives, SyncNoMultiple succeeds, oversized input returns OOM,
existing PCM continues, and GC/StopSounds releases retained voice storage.

## v0.788: real producer isolation and independent native workers

`flashvm_frontend_event_prepare_worker` wraps only the production owner's bounded
voice-open call with a 350 ms delay. During that delay the actual SDL worker must
submit over 10000 additional frames from the existing voice; a held producer
mutex would submit zero. Two real AVM1 frame actions check interpreter and native
JIT entry counters. GC and joined teardown run afterwards. This deliberately
stalls preparation, not a simulated queue or an inferred wall-clock DAC position.

`flashvm_frontend_voice_clone_workers` deep-clones four partially consumed voices,
collects/destroys the VM and closes the source template, then starts four actual
SDL threads. Each independently seeks/reads 1000 slices against analytical PCM,
then closes its own native handle before join. The test verifies independent
ownership, not unsynchronized access to one shared voice. SDL dummy/Linux results
are not hardware, ThreadSanitizer or foreign-platform acceptance.

## v0.789: reject fixed mixer storage before allocation

[`fvm_swf_sound_mixer_create_bounded`](../include/flashvm/swf_sound_mixer.h#L55) adds a fixed object/scratch/slot ceiling to
configured construction. Validate geometry first, then checked-size the exact
object and slot table before either allocation. Zero/short budget returns OOM
and null output; other configured errors propagate. No callback, device or VM.
Its storage_info.fixed_bytes equals the checked amount. Later incoming voices
still need bounded adoption. The desktop now uses this constructor before audio
device acquisition. Host template/queue and allocator memory remain separate.
`swf_sound_mixer_storage` additionally checks exact and one-byte-short creation.

## v0.790: runnable C89 template replay

The [swf_voice_reuse tutorial](../examples/multimedia/swf_voice_reuse/README.md)
prepares one bounded template, destroys its VM, parks it at EOF, then clones,
restarts and adopts it twice. Its aggregate budget reserves template, mixer and
incoming clone together. Native finite reads preserve authored loops/envelopes;
there is no repeat decoding or implicit movie/device clock. The independent
checker passes 24 FWS/CWS sample cases: mono/stereo PCM styles, empty playback,
ADPCM and six effective MPEG geometries. Both output halves match exactly;
FFmpeg checks every MP3 sample when installed. Errors/replacement are explicit.

## v0.791: actual three-codec event composition oracle

`flashvm_frontend_mp3_events` plays three overlapping root StartSound voices:
supported MP3, +30000 PCM and -28000 ADPCM. Every accepted SDL stereo sample is
compared with independently decoded FFmpeg MP3, analytical PCM/ADPCM and rational
conversion, followed by one saturation of the complete sum. Actual serialized
submission markers define each onset, avoiding wall-clock guesses. FWS/CWS and
both interpreter/native-JIT action counters run for all six MPEG geometries:
24 cases pass, maximum permitted deviation three S16 units. No optional oracle
was skipped in this run. This covers event composition after owner-lock changes;
root streaming plus StartSound, child dictionaries, reference timing and device
acceptance remain open. It does not redefine event onset as a movie/DAC clock.

## v0.792: failure boundaries and final evidence

`swf_sound_voice_storage_allocation` checks open, deep clone and fixed mixer
creation at isolated allocation allowances 0–3. Partial object/envelope/PCM
owners are reclaimed; failed outputs are null, and the existing template stays
at its exact repeat/frame cursor. GC follows every result, and the template
continues allocation-free after VM teardown. Mixer storage checks also verify
release tails remain charged, stop-by-ID clears the non-releasing activity
condition, and a new SyncNoMultiple voice can start while the old tail releases.

Final focused validation on this Linux host:

- 79 native sound/stream/codec and actual C89 usage/export checks passed.
- 14 real SDL dummy-player checks passed, including both MP3 oracles, frame gates,
  controls, preparation failures, budgets and four independent native readers.
- 11 full-minimal-core ASan/UBSan success-path/C89 checks passed. LeakSanitizer
  was disabled because this host cannot access /proc. Exception-injected
  allocation failures are not claimed under ASan.
- All 12 isolated allocation allowances passed ordinary execution and UBSan
  instrumentation of the changed voice/mixer modules; other core modules in that
  scoped failure run used the ordinary library.
- Deterministic documentation source links, sorted example catalog, version-only
  CMake refresh and changed tutorial comment/API coverage passed. The global
  multimedia comment checker still reports two unchanged baseline Windows speech
  files; their bytes match v0.782. It is not claimed globally passing.

This series does not complete SWF ≤ 8. Root stream-plus-event integration,
child dictionaries/streams, reference A/V and discontinuous seek, remaining
codecs/API/graphics/text/JIT/GC/debugger work and physical/foreign execution remain
open. Native deep-copy concurrency is not a claim of shared-handle thread safety,
ThreadSanitizer cleanliness, or reference Flash audible timing.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mixer_create_bounded` | function | [include/flashvm/swf_sound_mixer.h:55](../include/flashvm/swf_sound_mixer.h#L55) |
| `fvm_swf_sound_mixer_sound_active` | function | [include/flashvm/swf_sound_mixer.h:104](../include/flashvm/swf_sound_mixer.h#L104) |
| `fvm_swf_sound_mixer_storage_info` | function | [include/flashvm/swf_sound_mixer.h:98](../include/flashvm/swf_sound_mixer.h#L98) |
| `fvm_swf_sound_mixer_submit_prepared_bounded` | function | [include/flashvm/swf_sound_mixer.h:86](../include/flashvm/swf_sound_mixer.h#L86) |
| `fvm_swf_sound_voice_clone` | function | [include/flashvm/swf_sound.h:111](../include/flashvm/swf_sound.h#L111) |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:82](../include/flashvm/swf_sound.h#L82) |
| `fvm_swf_sound_voice_open_bounded` | function | [include/flashvm/swf_sound.h:97](../include/flashvm/swf_sound.h#L97) |
| `fvm_swf_sound_voice_seek` | function | [include/flashvm/swf_sound.h:123](../include/flashvm/swf_sound.h#L123) |
| `fvm_swf_sound_voice_storage_info` | function | [include/flashvm/swf_sound.h:101](../include/flashvm/swf_sound.h#L101) |
<!-- source-links:end -->
