# Caller-buffered prepared stream output

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
| [`fvm_swf_stream_output_init`](../include/flashvm/swf_stream_output.h#L44) | Copy backend and borrow caller native objects; empty queue and matching geometry required. INVALID_ARGUMENT for descriptors/bounds, UNSUPPORTED for insufficient double precision, backend statuses propagate. No allocation or source advancement. |
| [`fvm_swf_stream_output_pump`](../include/flashvm/swf_stream_output.h#L53) | Observe budget, prepare/retry one block, copy through backend. Write failure retains pending bytes; empty markers OK/0; EOF/backpressure NOT_FOUND. Accepted output clears first; errors may preserve a newly prepared pending block intentionally. |
| [`fvm_swf_stream_output_pause`](../include/flashvm/swf_stream_output.h#L68) | Acknowledge exactly 0/1 pause state; no source/FIFO clearing or priming. Failure preserves acknowledged pause. |
| [`fvm_swf_stream_output_info`](../include/flashvm/swf_stream_output.h#L72) | Copy clock/chunk only after a valid backlog observation. Exact struct_size required; error preserves output. No source read/allocation. |
| [`fvm_swf_stream_output_seek`](../include/flashvm/swf_stream_output.h#L77) | Validate source position, pause/clear, then reset source grid/pending/epoch. Staged failure contract above; invalid source positions trigger no backend side effects. |

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
| [`fvm_swf_stream_output_next_frame`](../include/flashvm/swf_stream_output.h#L57) | Copy pending/indexed next movie frame; source/backend ownership unchanged. No callback/VM/allocation. INVALID_ARGUMENT for null/reentry, NOT_FOUND for EOF; preserve output on failure. |
| [`fvm_swf_stream_output_pump_until`](../include/flashvm/swf_stream_output.h#L64) | Same retry/backpressure/accepted=0 contract as pump; max_movie_frame is explicit authorization, zero blocks all frames. Future/pending-too-new returns NOT_FOUND; no conversion/write for that block. Seek/clear remains the host's backward-navigation operation. |

Native contract tests cover gate zero, source frame gaps, failed future retries,
empty markers, pending precedence, unchanged positions/bytes and EOF with retained
FIFO data. Allocation injection covers next-frame/gated pumping after source
VM/runtime destruction. Full sound timeline/device/reference acceptance remains open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AudioQueueBackend` | struct | [include/flashvm/swf_sound_output.h:16](../include/flashvm/swf_sound_output.h#L16) |
| `fvm_swf_stream_output_info` | function | [include/flashvm/swf_stream_output.h:72](../include/flashvm/swf_stream_output.h#L72) |
| `fvm_swf_stream_output_init` | function | [include/flashvm/swf_stream_output.h:44](../include/flashvm/swf_stream_output.h#L44) |
| `fvm_swf_stream_output_next_frame` | function | [include/flashvm/swf_stream_output.h:57](../include/flashvm/swf_stream_output.h#L57) |
| `fvm_swf_stream_output_pause` | function | [include/flashvm/swf_stream_output.h:68](../include/flashvm/swf_stream_output.h#L68) |
| `fvm_swf_stream_output_pump` | function | [include/flashvm/swf_stream_output.h:53](../include/flashvm/swf_stream_output.h#L53) |
| `fvm_swf_stream_output_pump_until` | function | [include/flashvm/swf_stream_output.h:64](../include/flashvm/swf_stream_output.h#L64) |
| `fvm_swf_stream_output_seek` | function | [include/flashvm/swf_stream_output.h:77](../include/flashvm/swf_stream_output.h#L77) |
<!-- source-links:end -->
