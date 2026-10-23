# PCM output queues and dequeue clock

Optional queued identity tracking and copied deadlines: [QUEUED_SOUND_IDS.md](QUEUED_SOUND_IDS.md). Legacy untracked mixer submissions remain unchanged; tracked outputs require starts/stops through their native output APIs.

v0.581 adds an explicit host-owned bridge from the [StartSound mixer](SWF_SOUND_MIXER.md)
with optional [rate/channel conversion](SWF_SOUND_CONVERSION.md) to a replaceable
playback FIFO. [Русское руководство](SWF_SOUND_OUTPUT.ru.md).
[Annotated C89 tutorial](../examples/multimedia/swf_audio_queue/README.md).

## Ownership and execution

[`FVM_SWFSoundOutput`](../src/swf/sound_output.cpp#L11) owns one fixed PCM retry buffer. It borrows a mixer and copies
[`FVM_AudioQueueBackend`](../include/flashvm/swf_sound_output.h#L16); its userdata/device remains host-owned. Close output before
mixer/device, and keep userdata live through the final call. No context/managed
pointers are retained: prepared voices can outlive their source VM. No GC pin is
needed for these native handles; ordinary MovieClip roots still require their
usual [ownership](MOVIECLIP_SOUND.md).

Serialize all producer and mixer operations, either on one host control thread
or with an external mutex shared by every participating thread. A native worker
may pump prepared PCM under that mutex; submission from a context must remain on
that context's owning thread under the same mutex. The producer has exclusive
mixer-read and queue write/clear/pause ownership. Never close while a worker can
still access these objects: signal and join it first. Backend callbacks execute
on the invoking thread and must support the chosen host policy.
External writes, clears, mixer reads or seeks invalidate clock/accounting.
Callbacks must not mutate the borrowed mixer behind the producer. Reentry into
pump/info/pause/stop returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); callback-time close is ignored
to avoid freeing a live call. This guard is not a thread lock. Close outside callbacks.

## Backend contract

Set `struct_size` to sizeof the complete table and `abi_version` to
[`FVM_AUDIO_QUEUE_ABI_VERSION`](../include/flashvm/swf_sound_output.h#L10) (1). `sample_rate` and `channels` must exactly match
the mixer's geometry: 5512–192000 frames/s, one or two channels. `userdata` is a
borrowed opaque pointer. PCM is native-endian interleaved signed short, with
`channels` values per frame; all counts and clock positions are **frames**, not bytes.

| Callback | Required behavior |
| --- | --- |
| `write(userdata, pcm, frames)` | On [`FVM_OK`](../include/flashvm/flashvm.h#L42), copy/accept ALL frames before returning; never retain pcm. On any error/exception accept NONE. Partial writes must be resolved inside the bridge; reporting an error after partial acceptance would duplicate sound on retry. |
| `queued(userdata, out_frames)` | On success publish complete unconsumed FIFO frames, including while paused. Paused data stays queued. Report lost-device errors rather than falsely reporting an empty queue. |
| `pause(userdata, paused)` | Atomically set consumption to Boolean paused (1) or resumed (0). Error/exception preserves the prior state and data. |
| `clear(userdata)` | Discard all queued frames on success. On error/exception preserve data. Does not close the device. |

Callbacks may return appropriate [`FVM_Result`](../include/flashvm/flashvm.h#L40) errors, propagated unchanged.
C++ bad_alloc is caught as [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44); other exceptions become [`FVM_ERROR`](../include/flashvm/flashvm.h#L43).
The all-or-nothing rules apply to exceptions as well. All callbacks must finish
promptly; the API offers no asynchronous cancellation of a blocked native callback.
A backend that cannot honor this contract needs its own staging/transaction layer.

## Creation

`fvm_swf_sound_output_create(mixer, backend, block_frames, target_frames, out)`:
allocate the producer and its `block_frames * channels` sample buffer. Set *out to
null on failure (out itself must be nonnull). Require nonnull mixer/table and all
four callbacks, exact layout/version/geometry, block in 1..[`FVM_SWF_SOUND_OUTPUT_MAX_BLOCK`](../include/flashvm/swf_sound_output.h#L11)
(65536), and block <= target <= [`FVM_SWF_SOUND_OUTPUT_MAX_QUEUE`](../include/flashvm/swf_sound_output.h#L12) (262144).
Invalid input returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); allocation failure returns
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44). Hosts with DBL_MANT_DIG below [`FVM_SWF_SOUND_OUTPUT_CLOCK_BITS`](../include/flashvm/swf_sound_output.h#L13) (53) return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46).

Only after allocation does creation pause the backend and query its FIFO. The
queue must be empty; otherwise creation returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) without
clearing that PCM. A successful pause remains in effect even if the subsequent
query fails or finds existing data. Backend errors propagate. Success returns a
paused producer with a zero clock epoch; no voices or PCM have been consumed.
Mixer/device ownership is unchanged on success and failure.

## Pump and retry

`fvm_swf_sound_output_pump(output, accepted_frames)` observes the FIFO and processes
at most one block. The optional count output is zeroed first. Null/reentrant
output returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Invalid FIFO growth, a regressing consumed
clock, or exceeding [`FVM_SWF_SOUND_OUTPUT_MAX_CLOCK`](../include/flashvm/swf_sound_output.h#L14) (2^53−1 submitted frames per
epoch) returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Backend/query errors propagate.

A full target, or insufficient space for an already pending complete block,
returns [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L47), without another mixer read/write. Otherwise prepare up to
min(block, target−queued) frames (including silence after voice EOF), then write.
On success accepted_frames is that complete count. On failed write keep the exact
prepared PCM for the next explicit pump; the mixer has already advanced, and must
not be read again to retry. There is no automatic retry loop or rollback. Stop or
close discards pending PCM. Source-selective stop affects future mixing, not
samples already prepared/queued. To discard all such PCM, use output stop.

Allocation of producer PCM happens only at create. Mixer reads/accounting do not
allocate; completed voices may be freed and native backend writes may allocate.
Work is bounded by block size and active voices, not promised realtime-safe.

## Pause and copied clock

`fvm_swf_sound_output_pause(output, paused)` requires a live nonbusy producer and
0 or 1; otherwise [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). It calls the backend and updates cached
state only on success. It preserves FIFO/pending PCM and does not prime the queue.
Prime while paused, then explicitly resume. Backend errors propagate.

[`FVM_SWFSoundOutputInfo`](../include/flashvm/swf_sound_output.h#L34) stores the copied clock fields.
`fvm_swf_sound_output_info(output, info)` requires nonnull pointers, exact
`info.struct_size = sizeof(*info)` and nonbusy producer. Invalid input returns
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Refresh the FIFO and copy the complete struct on success;
errors leave the caller's struct unchanged. Backend errors propagate; impossible
FIFO growth or regressing clock returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L43).

| Field | Meaning |
| --- | --- |
| `struct_size` | Exact copied-info ABI layout size. |
| `submitted_frames` | Complete accepted frames since successful stop/reset, exact through 2^53−1. Includes accepted silence. |
| `queued_frames` | Last observed remaining complete frames. After pump, cached count is an upper bound until the next query because the consumer may run concurrently. |
| `played_frames` | submitted minus queued: FIFO-dequeue position, monotonically observed. |
| `seconds` | played_frames / output sample_rate. |
| `pending_frames` | Prepared complete block not yet accepted after a write failure. Not part of submitted or played. |
| `paused` | Last successful producer-controlled pause state. |
| `starved` | Nonpaused, some frames previously submitted, FIFO currently empty. No elapsed underrun duration is inferred. |

This clock measures software FIFO consumption, **not DAC position or exact audible
time**: driver/hardware buffering adds latency. During starvation it freezes until
more frames are consumed; it does not infer wall-clock silence, skip source samples
or catch up automatically. Hosts may use it for future scheduling, but must choose
latency/underrun policy explicitly. The desktop frontend is not automatically
connected to this API in this version.

## Stop and close

`fvm_swf_sound_output_stop(output)` rejects null/reentry with
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Pause first; on failure preserve prior state. After
successful pause, mark paused and clear the FIFO. If clear fails, return that
error while remaining paused; pending data, mixer voices and counters remain for
recovery. On success stop every mixer voice, discard pending data and reset every
clock count to zero, paused. Backend exceptions use the common translation above.
Host video/timeline scheduling must rebase after this new epoch. No seek of the
source MovieClip is performed.

`fvm_swf_sound_output_close(output)` accepts null and returns void. It releases
only producer memory/pending PCM: no callbacks, pause, clear, mixer stop, mixer
close or device close. Thus ownership can be released even after a lost device.
Stop first if coordinated silence/reset is required, then close producer, mixer,
and device. Reentrant close is ignored; it does not schedule deferred destruction.

## SDL2 native bridge

`fvm_audio_sdl_queue_open(rate, channels, device_buffer_frames, out, backend)` opens
the default playback device through real SDL2 APIs on Linux/Windows/macOS. Both
output pointers must be nonnull; handle and table are cleared before validation.
Rate 5512–192000 must fit int, channels 1..2, device buffer a power of two in
[`FVM_SDL_AUDIO_MIN_BUFFER`](../include/flashvm/swf_sound_output.h#L156)..[`FVM_SDL_AUDIO_MAX_BUFFER`](../include/flashvm/swf_sound_output.h#L157) (64..32768 frames).
Invalid input returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Non-8-bit bytes/non-16-bit short or
unexpected obtained format returns [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46); native init/open failure
returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L43); owner allocation failure returns [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44).
Every acquired partial resource is released on failure. Success publishes exact
native S16 geometry, a paused device and its borrowed callback table.

Uses SDL_InitSubSystem, SDL_OpenAudioDevice, SDL_QueueAudio,
SDL_GetQueuedAudioSize, SDL_PauseAudioDevice and SDL_ClearQueuedAudio, with
separate [operation files](../src/multimedia/audio/sdl_queue_open.cpp).
Device status STOPPED is reported as an error, not as completed playback.
SDL_QueueAudio success copies the entire block. SDL's dequeue observation remains
a software queue clock, not a hardware clock. No capture or microphone access.

[`FVM_SDLAudioQueue`](../src/multimedia/audio/sdl_queue.hpp#L11) owns the native SDL device.
`fvm_audio_sdl_queue_close(device)` accepts null ([`FVM_OK`](../include/flashvm/flashvm.h#L42)), closes the native
device and balances the acquired SDL audio-subsystem reference. Close output
first and do not call SDL_Quit while this device is live. Do not use the stale
table afterward. Nonnull device on a disabled build returns [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46);
valid device handles cannot be created in that configuration.


A native SDL queue-write error latches an adapter fault: subsequent queries/writes
and resume return [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) until explicit successful stop/clear. Pause remains
available. This avoids trusting SDK FIFO bookkeeping after allocation failures.
The pending producer block is retained, but this adapter requires reset rather
than an automatic retry. Close is always available.

Default builds have no SDL SDK dependency: valid open returns
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) with cleared outputs. Enable explicitly:

```sh
cmake -S flashVM -B /tmp/flashvm-audio -DFVM_ENABLE_SDL_AUDIO=ON -DFVM_BUILD_EXAMPLES=ON
cmake --build /tmp/flashvm-audio --target flashvm_examples_multimedia_swf_audio_queue_main
/tmp/flashvm-audio/flashvm_examples_multimedia_swf_audio_queue_main --sdl
```

Requires installed SDL2 >=2.0.9 with its CMake CONFIG package/SDL2::SDL2 target;
set SDL2_DIR for a nonstandard SDK. [Per-OS preparation](../examples/multimedia/swf_audio_queue/README.md#native-preparation).
`FVM_SDL_AUDIO_DEVICE_TESTS=ON` explicitly registers a real SDK queue protocol test
with SDL_AUDIODRIVER=dummy. Dummy tests are not audible hardware acceptance; native
Windows/macOS/Android output, source/movie synchronization and sustained underrun
recovery remain open. Existing FFmpeg decoding adapters are independent of this
output backend and are not replaced.

## Obtained device geometry (v0.583)

`fvm_audio_sdl_queue_info(device, info)` copies immutable obtained SDL geometry
into [`FVM_SDLAudioQueueInfo`](../include/flashvm/swf_sound_output.h#L165): `sample_rate` in Hz, `channels`, and
`device_buffer_frames` in interleaved frames per native device block. Initialize
`struct_size` to `sizeof(FVM_SDLAudioQueueInfo)`. The actual block may exceed the
request, particularly on Android; size reserve and resume thresholds from this
value. It is neither queue depth nor measured hardware latency.

A live owned device and nonnull correctly sized output are required. Invalid
pointers/size return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), preserving output. Success returns
[`FVM_OK`](../include/flashvm/flashvm.h#L42) without allocation, VM/GC access, callbacks or ownership transfer. The
immutable copy may be read on a worker while the owner guarantees device lifetime;
close must never race. Disabled builds return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) for otherwise
valid-shaped arguments; they cannot create a valid device.

## v0.586 block negotiation

[`fvm_audio_sdl_queue_open`](../include/flashvm/swf_sound_output.h#L160) now permits only SDL_AUDIO_ALLOW_SAMPLES_CHANGE so
`FVM_SDLAudioQueueInfo.device_buffer_frames` exposes the negotiated callback
block instead of a hidden requested-size conversion stream. Rate/channels/native
S16 at the queue input stay exact. The request keeps its power-of-two bounds;
the obtained nonzero quantum may be larger and need not be a power of two.
Use it to size queue reserve/resume, not as a DAC position or physical latency.
SDL SDK >=2.0.9 is required; the info struct/queue ABI and ownership are unchanged.
The explicit real-SDK quantum regression wraps acquisition with an enlarged
8192-frame device and checks that only sample-count negotiation is enabled.

## v0.587 optional stop release

[Player policy, new APIs and limitations](PLAYER_AUDIO_RELEASE.md). Original immediate stop behavior remains the default.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AUDIO_QUEUE_ABI_VERSION` | macro / define | [include/flashvm/swf_sound_output.h:10](../include/flashvm/swf_sound_output.h#L10) |
| `fvm_audio_sdl_queue_close` | function | [include/flashvm/swf_sound_output.h:176](../include/flashvm/swf_sound_output.h#L176) |
| `fvm_audio_sdl_queue_info` | function | [include/flashvm/swf_sound_output.h:174](../include/flashvm/swf_sound_output.h#L174) |
| `fvm_audio_sdl_queue_open` | function | [include/flashvm/swf_sound_output.h:160](../include/flashvm/swf_sound_output.h#L160) |
| `FVM_AudioQueueBackend` | struct | [include/flashvm/swf_sound_output.h:16](../include/flashvm/swf_sound_output.h#L16) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `FVM_SDL_AUDIO_MAX_BUFFER` | macro / define | [include/flashvm/swf_sound_output.h:157](../include/flashvm/swf_sound_output.h#L157) |
| `FVM_SDL_AUDIO_MIN_BUFFER` | macro / define | [include/flashvm/swf_sound_output.h:156](../include/flashvm/swf_sound_output.h#L156) |
| `FVM_SDLAudioQueue` | struct | [src/multimedia/audio/sdl_queue.hpp:11](../src/multimedia/audio/sdl_queue.hpp#L11) |
| `FVM_SDLAudioQueueInfo` | struct | [include/flashvm/swf_sound_output.h:165](../include/flashvm/swf_sound_output.h#L165) |
| `FVM_SWF_SOUND_OUTPUT_CLOCK_BITS` | macro / define | [include/flashvm/swf_sound_output.h:13](../include/flashvm/swf_sound_output.h#L13) |
| `fvm_swf_sound_output_close` | function | [include/flashvm/swf_sound_output.h:153](../include/flashvm/swf_sound_output.h#L153) |
| `fvm_swf_sound_output_create` | function | [include/flashvm/swf_sound_output.h:50](../include/flashvm/swf_sound_output.h#L50) |
| `fvm_swf_sound_output_info` | function | [include/flashvm/swf_sound_output.h:138](../include/flashvm/swf_sound_output.h#L138) |
| `FVM_SWF_SOUND_OUTPUT_MAX_BLOCK` | macro / define | [include/flashvm/swf_sound_output.h:11](../include/flashvm/swf_sound_output.h#L11) |
| `FVM_SWF_SOUND_OUTPUT_MAX_CLOCK` | macro / define | [include/flashvm/swf_sound_output.h:14](../include/flashvm/swf_sound_output.h#L14) |
| `FVM_SWF_SOUND_OUTPUT_MAX_QUEUE` | macro / define | [include/flashvm/swf_sound_output.h:12](../include/flashvm/swf_sound_output.h#L12) |
| `fvm_swf_sound_output_pause` | function | [include/flashvm/swf_sound_output.h:136](../include/flashvm/swf_sound_output.h#L136) |
| `fvm_swf_sound_output_pump` | function | [include/flashvm/swf_sound_output.h:124](../include/flashvm/swf_sound_output.h#L124) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:142](../include/flashvm/swf_sound_output.h#L142) |
| `FVM_SWFSoundOutput` | struct | [src/swf/sound_output.cpp:11](../src/swf/sound_output.cpp#L11) |
| `FVM_SWFSoundOutputInfo` | struct | [include/flashvm/swf_sound_output.h:34](../include/flashvm/swf_sound_output.h#L34) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
