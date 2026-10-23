# Caller-buffered prepared stream output

Additive authored MP3 binding/seek APIs are documented in
[prepared frame seek](MP3_FRAME_SEEK.md). Legacy block-start navigation retains
its behavior. Hosts still own frame authorization and device queue scheduling.


[Русский](STREAM_PCM_OUTPUT.ru.md), [conversion](STREAM_PCM_CONVERSION.md).
`swf_stream_output.h` connects a prepared native converter to the existing
[FVM_AudioQueueBackend](../include/flashvm/swf_sound_output.h#L16) contract. The producer, retry buffer and converter are
caller-owned; init/pump/pause/info/seek allocate nothing and never access a VM.
This is bounded FIFO production, not desktop SWF stream timeline playback.

## Ownership, backpressure and retries

Bind a converter at its current position, an exclusive empty backend queue and
a disjoint scalar S16 buffer. Block size is 1..1024 output frames, target is at
least block and at most SOUND_OUTPUT_MAX_QUEUE. Buffer capacity counts scalar
values, at least block*output_channels. Geometry must match converter/backend.
Init copies the backend table, borrows its userdata, validates empty backlog and
pauses on success. Init failure preserves caller state/converter/PCM; init reentry
is forbidden. Native callback failures propagate; C++ exceptions become ERROR.

The host retains snapshot/converter/buffer/backend until all producer operations
and callbacks stop. Do not independently seek/read the borrowed converter, alter
retry PCM, or write/clear the backend. Serialize all producer calls; busy protection
rejects callback reentry, but is not a mutex. Dispose requires only quiescence,
then explicit host queue/device cleanup; there is no hidden close or destructor.

Pump prepares at most one converted chunk and respects available target budget.
A failed write keeps exact pending PCM and chunk: converter has advanced once,
retry never decodes or advances it again. Backend write must copy all frames
before OK and accept none on any failure/exception. Arbitrary partially written
files do not meet this contract; use a bounded owned FIFO before irreversible I/O.
Empty/skipped source markers return OK/accepted=0 and update chunk metadata.
NOT_FOUND means backpressure or observed source EOF; query eof to distinguish.
The optional accepted output is cleared first, including invalid arguments/errors.

## Queue clocks and explicit seek

Info observes exclusive backlog: queued cannot exceed target/submitted and
submitted-queued cannot regress. Violations return ERROR, preserving caller output.
submitted_frames and played_frames are exact double counts through MAX_CLOCK;
seconds is played_frames/output_rate. This is dequeue accounting, not DAC position
or a movie/A/V clock. EOF may be true while final accepted audio is still queued.
Pause/resume changes only backend state; it does not prime, clear or advance data.

Seek validates an absolute source block/offset using a copied converter before
callbacks. It then pauses and clears FIFO. Only successful clear publishes new
source position/phase zero, discards pending, and resets the accounting epoch.
If pause succeeds but clear fails, pause remains acknowledged; pending/source and
old epoch stay intact for retry. Successful seek leaves paused; the host primes
and resumes explicitly. Already heard audio cannot be rolled back.

| API | Contract |
| --- | --- |
| [`fvm_swf_stream_output_init`](../include/flashvm/swf_stream_output.h#L52) | Copy backend and borrow caller native objects; empty queue and matching geometry required. INVALID_ARGUMENT for descriptors/bounds, UNSUPPORTED for insufficient double precision, backend statuses propagate. No allocation or source advancement. |
| [`fvm_swf_stream_output_pump`](../include/flashvm/swf_stream_output.h#L61) | Observe budget, prepare/retry one block, copy through backend. Write failure retains pending bytes; empty markers OK/0; EOF/backpressure NOT_FOUND. Accepted output clears first; errors may preserve a newly prepared pending block intentionally. |
| [`fvm_swf_stream_output_pause`](../include/flashvm/swf_stream_output.h#L114) | Acknowledge exactly 0/1 pause state; no source/FIFO clearing or priming. Failure preserves acknowledged pause. |
| [`fvm_swf_stream_output_info`](../include/flashvm/swf_stream_output.h#L126) | Copy clock/chunk only after a valid backlog observation. Exact struct_size required; error preserves output. No source read/allocation. |
| [`fvm_swf_stream_output_seek`](../include/flashvm/swf_stream_output.h#L144) | Validate source position, pause/clear, then reset source grid/pending/epoch. Staged failure contract above; invalid source positions trigger no backend side effects. |

## Build and evidence

```sh
cmake -S flashVM -B build-stream-output -DFVM_BUILD_TESTS=ON
cmake --build build-stream-output --target flashvm_tests_swf_stream_output \
  flashvm_tests_swf_stream_output_allocation flashvm_tests_swf_stream_pcm_reader_api
ctest --test-dir build-stream-output -R 'swf_stream_(output.*|pcm_reader_api)$' --output-on-failure
```

Native tests cover all-or-none failed/thrown writes, exact retry data, callback
reentry, budget backpressure, consumed clocks, impossible backlog/regression,
empty/EOF markers, seek/clear failure and pending cancellation. Allocation injection
forbids new allocation after source VM/runtime teardown. A model backend proves
these contracts; it is not audible-device or full player/platform acceptance.

## Executable native FIFO/file adapter (v0.754)

The [C89 swf_stream tutorial](../examples/multimedia/swf_stream/README.md)
adds `OUTPUT.pcm --queue`. The annotated [queue.inc](../examples/multimedia/swf_stream/queue.inc)
is part of that executable, not a sketch. It binds the prepared 48000 Hz stereo
converter to a two-frame caller-owned FIFO. Backend write only copies into native
storage, atomically; it never writes the file. Once full, or at source EOF, the
serial host consumer drains the FIFO through actual fwrite. A file failure aborts
without pretending the producer can retry a partially written external file.

The source VM/runtime have already been destroyed; native snapshot, retry storage,
FIFO and callback userdata remain owned until producer/consumer quiescence. Empty
blocks remain visible in the source report. EOF plus a nonempty FIFO is not treated
as completion: final bytes must drain before pause and file close. Output and report
match `--convert` exactly (four stereo frames, sixteen raw native-endian bytes).
The independent integer oracle checks all bytes; failed open/cleanup is tested.

This example starts no audio device, thread or socket. It demonstrates a real
bounded native FIFO and actual file consumption, not audible output, real-time
queue scheduling, a DAC clock or SWF timeline/A/V synchronization.

```sh
cmake -S flashVM -B build-stream -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-stream --target flashvm_examples_multimedia_swf_stream_main
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --queue
python3 flashVM/scripts/check_swf_stream_example.py ./build-stream/flashvm_examples_multimedia_swf_stream_main
```

Validation at v0.754: 75 selected native regression checks and 14 shared-host checks pass. New output/converter/reader/snapshot paths, worker contracts and all five export modes pass ASan/UBSan; allocation-injection contracts pass natively. Leak detection is disabled by host restrictions; whole-VM instrumentation is not claimed. New C++98 implementations and C89 consumer/tutorial objects compile for ARM64 Linux/musl, RISC-V64 Linux/musl and Windows x64/MinGW; no foreign execution or full cross-built player/device acceptance. All nine roadmap stages remain open.

## Explicit movie-frame authorization (v0.755)

The unrestricted pump remains available. A new gated pump accepts only source
blocks with a movie-frame number at or below a host-provided boundary. Zero holds
all frames. It checks trusted prepared metadata before conversion, or pending
retry metadata before another write. A future frame returns NOT_FOUND without
advancing the converter or touching its PCM; queue-clock observations may still
update. No blocked future data is dropped. Empty/skipped markers obey the same
boundary, preserving gaps rather than silently jumping to later audio.

The separate next-frame query copies pending frame first, otherwise the indexed
source block frame. It performs no backend callbacks, decode, allocation or VM
access. NOT_FOUND at source EOF preserves the output frame. Combined with info,
it distinguishes future-frame gating, FIFO backpressure and EOF. A gate does not
recall data already accepted into FIFO. Backward navigation/new root incarnations
require explicit source seek/queue clear or new native objects; a decreasing tick
counter alone is never a safe synchronization policy. The host supplies actual
movie-frame authorization, not host tick/presentation counts.

| API | Ownership and errors |
| --- | --- |
| [`fvm_swf_stream_output_next_frame`](../include/flashvm/swf_stream_output.h#L65) | Copy pending/indexed next movie frame; source/backend ownership unchanged. No callback/VM/allocation. INVALID_ARGUMENT for null/reentry, NOT_FOUND for EOF; preserve output on failure. |
| [`fvm_swf_stream_output_pump_until`](../include/flashvm/swf_stream_output.h#L74) | Same retry/backpressure/accepted=0 contract as pump; max_movie_frame is explicit authorization, zero blocks all frames. Future/pending-too-new returns NOT_FOUND; no conversion/write for that block. Seek/clear remains the host's backward-navigation operation. |

Native contract tests cover gate zero, source frame gaps, failed future retries,
empty markers, pending precedence, unchanged positions/bytes and EOF with retained
FIFO data. Allocation injection covers next-frame/gated pumping after source
VM/runtime destruction. Full sound timeline/device/reference acceptance remains open.

## Actual MovieClip owner integration (v0.756)

The [C swf_stream tutorial](../examples/multimedia/swf_stream/README.md) adds
`OUTPUT.pcm --timeline`. Unlike native-only modes, it retains the source context
and imports the exact same movie as a pinned root with explicit stream omission.
PCM stays in the independent prepared snapshot/FIFO. The host advances MovieClip,
reads its actual _currentframe on the owner, and supplies that number to the gated
producer. It never substitutes host ticks for source frame identity. During a
controlled stop, two real ticks leave frame 1 unchanged and emit no duplicate or
future PCM. Play/resume is explicit. A source frame gap waits for the later actual
frame; empty markers retain their own boundary. Root/library stay pinned through
owner GC steps, with unpin before context destruction after producer quiescence.

The file and human report match `--queue`/`--convert` exactly, including all four
converted stereo frames and the explicit empty block. This serial file tutorial
models authorization/ownership, not paced sound playback. It opens no audio device,
worker or window and does not install automatic desktop stream playback.

A separate raw SWF integration test gives every frame an AVM1 assignment. It runs
both interpreter and native JIT when available, checks visited frame values and
requires positive native_entries in JIT mode. Actual stop/play ticks, source gaps,
GC safe points and gotoAndStop backwards are checked. Backward navigation pairs
with native seek/clear before priming again. Seeking directly to frame 3 starts a
new rational grid, so its isolated output duration differs intentionally from the
continuous forward grid; the test checks that reset instead of assuming continuity.

This is bounded owner integration, not reference-player stream semantics, child
streams, multi-segment scheduling, stream/event mixing, codec support, DAC clocks
or A/V synchronization. Unmuted desktop root stream playback remains unsupported.

```sh
cmake -S flashVM -B build-stream -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-stream --target flashvm_examples_multimedia_swf_stream_main
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --timeline
python3 flashVM/scripts/check_swf_stream_example.py ./build-stream/flashvm_examples_multimedia_swf_stream_main
```

The source context, clip/prototypes and all fvm_object_* / MovieClip / GC calls
belong strictly to the VM owner thread. Backend callbacks/native audio workers
must receive copied scalar authorization, not enter the VM or borrow clip pointers.
A host must not authorize PCM until its own frame actions/transitions have reached
the intended safe point; failed ticks/actions may leave committed effects.

## Explicit frame-based source seek

[`fvm_swf_stream_output_seek_frame`](../include/flashvm/swf_stream_output.h#L150) accepts a positive movie frame and binary-searches
the immutable segment index for its first block at/after that frame, then delegates
to the existing staged pause/clear/seek. A frame gap selects the later block and
the gate still holds it until authorized. Beyond the last block selects source EOF.
The rational grid restarts with phase zero. No decode, allocation or VM lookup is
needed; invalid zero/reentry returns INVALID_ARGUMENT before backend effects.

A gate is an upper bound, not an exact-frame selector: raising it after a forward
jump would also permit earlier unconsumed blocks. Hosts that discard skipped audio
must pair nonsequential forward/backward MovieClip navigation with explicit source
seek/clear. This helper defines that prefix-discard policy; it does not emulate
all reference Flash stream-seeking semantics or select another header segment.
Clear/pause failures retain the same staged ownership contract as block seek.
The actual interpreter/native-JIT integration test uses this helper for gotoAndStop.

| API | Ownership and errors |
| --- | --- |
| [`fvm_swf_stream_output_seek_frame`](../include/flashvm/swf_stream_output.h#L150) | Resolve first indexed block >= positive frame without VM/allocation, then pause/clear/reset epoch. Gaps wait, past end seeks EOF. Invalid metadata/arguments cause no backend effects; later callback failures obey staged seek semantics. |

Validation at v0.756: 77 selected native checks pass (one previously built MP3 test was rerun after restoring its executable permission), plus 14 shared-host checks. Updated frame-seek/gating/C89 contracts also pass the focused final suite. ASan/UBSan covers native output/reader/snapshot paths and actual MovieClip authorization with interpreter/native-JIT actions; whole-VM instrumentation and leak detection are not claimed (host disables leak checks). All six export modes compare exact PCM; the minimal build works without desktop frontend/device. New C++98/C89 implementation/consumer/tutorial objects compile for ARM64 Linux/musl, RISC-V64 Linux/musl and Windows x64/MinGW; no foreign execution/full cross-player acceptance. All nine roadmap stages remain open.


## Intrinsic timeline observation (v0.757)

`fvm_movieclip_get_playback_info(context, clip, &info)` reads private local timeline state rather than replaceable `_currentframe` properties. Set `info.struct_size = sizeof(info)` first. The [`FVM_MovieClipPlaybackInfo`](../include/flashvm/movieclip.h#L69) result contains positive `frame` and `total_frames`, plus `playing` and `entered` flags. Construction and goto select a frame with `entered=0`; the first subsequent actual entry changes it to one. Selection alone must not authorize a stream block. A stopped clip still receives onEnterFrame on later ticks. Host pause is a separate policy.

Call only on the VM owner thread, outside collection/teardown, retaining the same-context native clip for the call. No allocation, callback, script lookup, tick or managed pointer escapes; copied scalars need no root. Null/wrong-sized/foreign/prototype/corrupt input returns INVALID_ARGUMENT and preserves the entire descriptor. The query is not a navigation event log: multiple changes between observations may be invisible. The host still owns source identity, queue discontinuity and synchronization policy. `movieclip_playback_info.cpp` verifies invalid-output atomicity, zero-allocation observation, stopped ticks, seek/entry distinction, looping, replaced script property and GC-safe retained ownership.

## Multiple prepared sources (v0.760)

`fvm_swf_stream_output_continue(output, next)` requires an exhausted current
source with no pending PCM and a distinct initialized converter whose output
rate/channels match the backend. The next converter is borrowed at its current
position; source rates/channels may differ. Success changes only the binding
and clears EOF. Accepted FIFO samples, submitted/played clocks, pause and last
chunk remain intact; empty sources can be continued in bounded host-controlled
steps. No allocation, VM, backend callback or implicit decode occurs. Host owns
ordering, conversion phase per source, source lifetimes and external serialization.
INVALID_ARGUMENT covers unfinished/pending/self/null/mismatched/reentrant calls;
other validation errors propagate, preserving both cursors, FIFO and descriptor.

`fvm_swf_stream_output_bind_frame(output, target, positive_frame)` is an explicit
discontinuity: validate geometry and select the first target block at/after the
frame, then pause and clear. Only acknowledged clear publishes target binding,
its phase-zero position and reset output clock. Same converter is allowed. On
failed clear the prior converter, pending bytes and target cursor remain unchanged;
an acknowledged pause is retained. Errors/exception statuses follow ordinary seek.
Beyond-last seeks EOF, gaps select later blocks. Caller exclusively retains the
new converter/snapshot until producer quiescence. No allocation or VM calls.
This is a host prefix-discard policy, not reference Flash cross-header seeking.

`swf_stream_output_segments.cpp` checks exact PCM, empty intermediates, different
source rate/channel geometry, nonempty FIFO continuity, callback-free transition,
zero-allocation calls after VM teardown, invalid output geometry, reentry and
failed-clear preservation of pending retry bytes and both source positions.


Gated mixed scheduling is additive: [fvm_swf_stream_output_pump_mix_until](../include/flashvm/swf_stream_output.h#L111) handles authorized base and held/EOF events with ordered retry. Original stream-only entry points retain their contracts. [Current contract](MIXED_GATES.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AudioQueueBackend` | struct | [include/flashvm/swf_sound_output.h:16](../include/flashvm/swf_sound_output.h#L16) |
| `fvm_movieclip_get_playback_info` | function | [include/flashvm/movieclip.h:83](../include/flashvm/movieclip.h#L83) |
| `FVM_MovieClipPlaybackInfo` | struct | [include/flashvm/movieclip.h:69](../include/flashvm/movieclip.h#L69) |
| `fvm_swf_stream_output_bind_frame` | function | [include/flashvm/swf_stream_output.h:163](../include/flashvm/swf_stream_output.h#L163) |
| `fvm_swf_stream_output_continue` | function | [include/flashvm/swf_stream_output.h:156](../include/flashvm/swf_stream_output.h#L156) |
| `fvm_swf_stream_output_info` | function | [include/flashvm/swf_stream_output.h:126](../include/flashvm/swf_stream_output.h#L126) |
| `fvm_swf_stream_output_init` | function | [include/flashvm/swf_stream_output.h:52](../include/flashvm/swf_stream_output.h#L52) |
| `fvm_swf_stream_output_next_frame` | function | [include/flashvm/swf_stream_output.h:65](../include/flashvm/swf_stream_output.h#L65) |
| `fvm_swf_stream_output_pause` | function | [include/flashvm/swf_stream_output.h:114](../include/flashvm/swf_stream_output.h#L114) |
| `fvm_swf_stream_output_pump` | function | [include/flashvm/swf_stream_output.h:61](../include/flashvm/swf_stream_output.h#L61) |
| `fvm_swf_stream_output_pump_mix_until` | function | [include/flashvm/swf_stream_output.h:111](../include/flashvm/swf_stream_output.h#L111) |
| `fvm_swf_stream_output_pump_until` | function | [include/flashvm/swf_stream_output.h:74](../include/flashvm/swf_stream_output.h#L74) |
| `fvm_swf_stream_output_seek` | function | [include/flashvm/swf_stream_output.h:144](../include/flashvm/swf_stream_output.h#L144) |
| `fvm_swf_stream_output_seek_frame` | function | [include/flashvm/swf_stream_output.h:150](../include/flashvm/swf_stream_output.h#L150) |
<!-- source-links:end -->
