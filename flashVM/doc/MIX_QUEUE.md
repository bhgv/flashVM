# Retained stream/event queue composition (v0.772)

Optional queued sound identities now cover pending and accepted stream/event output: [EN](MIX_QUEUE_IDENTITIES.md) / [RU](MIX_QUEUE_IDENTITIES.ru.md). This opt-in mode adds budgeted deadline storage, tracked admission/query/stop and acknowledged full clear that stops tracked mixer voices; default untracked behavior is unchanged.

Opt-in shared base/tail frame accounting is now available: [EN](MIX_QUEUE_CLOCK.md) / [RU](MIX_QUEUE_CLOCK.ru.md). Explicit begin requires no pending retry and an empty FIFO; accepted-minus-queued observations cover both producers and end requires drain. Existing public structures and unclocked behavior are preserved.

Current additive tail and byte-budget APIs: [EN](AUDIO_MIX_TAILS.md) / [RU](AUDIO_MIX_TAILS.ru.md). After actual producer quiescence, pump_tail owns its zero-base retry independently of adapted.write. Backend NOT_FOUND may mean a retained rejected block; inspect pending origin rather than treating that status as EOF. Desktop stream/event integration remains separate.

[Русский](MIX_QUEUE.ru.md) · [additive mixer](STREAM_EVENT_MIX.md) · [C89 example](../examples/multimedia/swf_stream_mix/README.md)

The native composition adapter wraps one exclusive all-or-none PCM queue backend.
It copies a new input block, mixes event voices once, and retains **both original
input and mixed output** until the real queue accepts the complete mixed block.
A retry must match original frame count and every original byte. It resends the
retained mixture without advancing voices or mixing the base again. The wrapper
can be passed directly to the existing native stream-output producer.

`fvm_swf_sound_mix_queue_create(mixer, backend, block_frames, out, adapted)`
allocates the native owner and two fixed arrays, copies backend geometry and
returns an ordinary [FVM_AudioQueueBackend](../include/flashvm/swf_sound_output.h#L16) table. It does not call/pause/clear the
device, execute a VM, create a thread or take a mixer/device ownership reference.
The mixer is borrowed for exclusive rendering; event submissions must be serialized.
Both outputs must be nonnull/disjoint. *out is cleared first, adapted is unchanged
on failure. Backend and adapted may be the same descriptor storage. A null output,
bad ABI/callback table, null mixer, geometry mismatch or block outside
1..[FVM_SWF_SOUND_OUTPUT_MAX_BLOCK](../include/flashvm/swf_sound_output.h#L11) returns INVALID_ARGUMENT. Size overflow or
allocation failure returns OUT_OF_MEMORY and frees partial ownership.

Array storage requests total `2 * block_frames * channels * sizeof(short)`, plus
the native object; mixer/voice/device storage and allocator bookkeeping are separate.
No arrays grow during callback use. The adapter and borrowed mixer may outlive the
original source context/runtime. The host must keep mixer, real backend userdata
and adapter alive until all table users are quiescent.

## Callback contract

| Wrapped operation | Behavior |
| --- | --- |
| write, no pending block | Copy base, call mixer_add once, retain mixed frames, call real write |
| write, pending block | Require identical original bytes/count; resend mixed bytes only |
| write OK | Release pending ownership; actual backend copied every frame |
| write error/exception | Retain mixture and consumed voice time; exception becomes ERROR |
| queued | Forward real count; preserve caller output on failure/exception |
| pause | Forward valid zero/one policy; preserve pending block |
| clear | Discard pending only after real clear returns OK; never rewind event voices |

Positive input must be valid initialized S16 at configured geometry and within the
fixed frame capacity. Zero frames is an allocation/callback-free OK no-op only
when no retry is pending; otherwise INVALID_ARGUMENT. A mismatched retry returns
INVALID_ARGUMENT before calling the real backend or moving voice time. Recursive
wrapped calls/observations during a backend callback return INVALID_ARGUMENT.
Backend callback statuses propagate, and C++ exceptions become ERROR with reentry
protection restored. This relies on the real backend's existing atomic failure
contract: accepted data or acknowledged device policy cannot be rolled back if
that backend violates its contract.

`fvm_swf_sound_mix_queue_info(adapter, out)` copies exact native struct_size,
capacity_frames and pending_frames without device observation, allocation or VM
access. Null/wrong-size/reentry returns INVALID_ARGUMENT and preserves output.
`fvm_swf_sound_mix_queue_close(adapter)` frees native arrays/object without callbacks,
voice stop or device close; null is allowed. Caller must quiesce all callback-table
users first. Every operation needs external serialization; the busy flag prevents
callback recursion and is not a thread lock. No managed pointer is retained.

## Producer, cancellation and tails

Create the adapter before [`fvm_swf_stream_output_init`](../include/flashvm/swf_stream_output.h#L52), passing its returned table
to that producer. The producer owns original stream retry PCM, while the adapter
owns its already mixed retry. On write rejection both layers retain the same
logical block. Queue clocks/frame gates remain the producer's responsibility.
Only this chain may write or clear the real queue while that producer is active.

A successful clear cancels mixed pending/queued data but **does not restore voice
time** already rendered. Coordinate it with producer seek/reset and the desired
mixer stop/event policy. Calling the real backend clear directly bypasses the
adapter and violates ownership. Closing the adapter is not a stop operation.
Events submitted after a rejection affect later newly composed blocks, never the
immutable pending mixture. They cannot retroactively change accepted audio.

At source EOF, a host may quiesce its stream producer and explicitly submit zero
base for event tails. It must not then query/pump an old exclusive producer as if
those extra frames belonged to its submitted-frame clock. A general desktop host
still needs one coordinated stream/event timeline, stop/seek and tail policy;
desktop mixed movies remain unsupported in this checkpoint. No MP3, child-stream
or A/V synchronization support is claimed.

## Executable checks

The actual C89 [tutorial](../examples/multimedia/swf_stream_mix/README.md) adds
`OUTPUT.pcm --queue`. A real bounded native FIFO rejects each new block once before
copying it. The adapter retries the original base; all 16 stereo output frames,
including event tail and final silence, equal the direct export. A separate file
consumer runs only after queue acceptance; fwrite errors are terminal, not fake
atomic queue errors. Source VM teardown precedes all native production.

```sh
cmake --build build --target flashvm_tests_swf_sound_mix_queue \
  flashvm_tests_swf_sound_mix_queue_allocation flashvm_tests_swf_sound_mix_queue_c89 \
  flashvm_examples_multimedia_swf_stream_mix_main
ctest --test-dir build --output-on-failure -R 'swf_sound_mix_queue|swf_stream_mix_tutorial'
./build/flashvm_examples_multimedia_swf_stream_mix_main mixed.pcm --queue
```

The integration fixture uses an actual prepared stream producer, two events from
one SWF, failed/throwing writes, mismatched retry, callback recursion, failed count
observation, clear acknowledgement and a separately authorized tail. Independent
absolute-rational tutorial checks compare every sample in direct/queue modes.
Allocation injection covers object/two-array acquisition and zero-allocation
rejection/retry/observation/clear after source teardown. These bounded contracts
leave full Flash, concurrency and physical/platform acceptance open.

## Validation boundary for v0.772

43 native sound/stream regressions and the exact C89 tutorial pass. ASan/UBSan
instrumentation covers mixer, composition adapter, producer and new fixtures;
the remaining core/dependencies are not fully instrumented. Leak enumeration is
disabled on this restricted host, fault budgets run in separate processes and
the throwing-callback path is exercised in the ordinary native test.

All three new fixtures plus the tutorial cross-link for Linux ARM64, RISC-V64,
x64 musl and Windows x64. Twelve ELF64 CPU/static identities and four PE32+ direct
OS import sets pass inspection. Only native Linux x64 and x64 musl binaries run
here; foreign CPUs, Windows, real audio devices and full desktop mixed playback
remain unaccepted. This revision adds a static-core C API, not automatic exports
from every embedding host shared library.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AudioQueueBackend` | struct | [include/flashvm/swf_sound_output.h:16](../include/flashvm/swf_sound_output.h#L16) |
| `fvm_swf_sound_mix_queue_close` | function | [include/flashvm/swf_sound_mix_queue.h:279](../include/flashvm/swf_sound_mix_queue.h#L279) |
| `fvm_swf_sound_mix_queue_create` | function | [include/flashvm/swf_sound_mix_queue.h:215](../include/flashvm/swf_sound_mix_queue.h#L215) |
| `fvm_swf_sound_mix_queue_info` | function | [include/flashvm/swf_sound_mix_queue.h:275](../include/flashvm/swf_sound_mix_queue.h#L275) |
| `FVM_SWF_SOUND_OUTPUT_MAX_BLOCK` | macro / define | [include/flashvm/swf_sound_output.h:11](../include/flashvm/swf_sound_output.h#L11) |
| `fvm_swf_stream_output_init` | function | [include/flashvm/swf_stream_output.h:52](../include/flashvm/swf_stream_output.h#L52) |
<!-- source-links:end -->
