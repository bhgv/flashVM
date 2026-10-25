# Prepared-output StopSounds bindings — v0.583

[Русское руководство](AUDIO_STOP_BINDINGS.ru.md).
[`FVM_AudioStopBinding`](../src/multimedia/audio/channel.hpp#L15) connects a host-prepared mixer/queue to the context's
AVM1 StopSounds, standard stopAllSounds function and [`fvm_audio_stop_all`](../include/flashvm/multimedia.h#L120).
A prepared voice can exist before its first PCM write; existing PCM-channel
activity remains unchanged. No fake frame submission is necessary.
[Runnable C89 control example](../examples/multimedia/stop_binding/README.md).
[Real player demo](../examples/player/animation_audio/03-stop-sounds.swf).

## Callback and order

[`FVM_AudioStopCallback`](../include/flashvm/audio_stop.h#L13) receives borrowed native userdata and returns [`FVM_Result`](../include/flashvm/flashvm.h#L42).
It must idempotently stop/discard all prepared voices, pending PCM and native
queue state it owns. No global device is implicitly opened. No PCM format, mixer
implementation or OS is imposed by the binding. The SDL player uses
[`fvm_swf_sound_output_stop`](../include/flashvm/swf_sound_output.h#L184) to pause/clear/reset its explicit output.

[`fvm_audio_stop_all`](../include/flashvm/multimedia.h#L120) first attempts each active PCM channel in registration order,
then **every** binding in binding-registration order, even if no PCM was accepted.
Empty/inactive PCM channels still do not receive redundant stops. Bindings always
receive the call: the VM cannot inspect an external mixer's preparation state.
Success does not detach a binding; later calls and context destruction invoke it
again. All callbacks must tolerate already stopped state.

Every callback is attempted after earlier failures. The first non-OK result in
that order is returned. A C++ bad_alloc becomes [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46); any other
exception becomes [`FVM_ERROR`](../include/flashvm/flashvm.h#L45), independently per callback. The same boundary now
covers channel stop/close and terminal channel cleanup. Callback failures do not
remove bindings; explicit later stop retries them. No automatic loop or rollback
of earlier successful device stops is performed.

## Create

`fvm_audio_stop_binding_create(context, callback, userdata, out)` copies callback
and userdata into a native registration, publishes a context-owned opaque handle,
and returns [`FVM_OK`](../include/flashvm/flashvm.h#L44). It invokes no callback and retains no managed value.
`out` must be nonnull; *out is cleared on every failure. Context must be live,
owned by the calling thread, outside GC/teardown/audio callbacks, and callback
must be nonnull. Invalid arguments/phases return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47).
`userdata` may be null if the callback accepts it.

[`FVM_AUDIO_STOP_MAX_BINDINGS`](../include/flashvm/audio_stop.h#L9) (64) limits registrations independently from PCM
channels. Reaching capacity or allocation failure returns [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46);
other native exceptions return [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Allocation/insertion failures publish
no entry and leave existing registrations unchanged. A failed creation transfers
no ownership. Successful registration belongs to this context, not the runtime,
source movie, global function value or another cloned context.

## Close

`fvm_audio_stop_binding_close(binding)` unregisters and destroys that wrapper,
returning [`FVM_OK`](../include/flashvm/flashvm.h#L44). Null is accepted. It **does not invoke stop**, so detachment
remains possible after an unrecoverable device failure. Stop explicitly first if
its result matters, then detach before releasing the borrowed native mixer/device/
userdata. A valid binding in an invalid context phase or callback returns
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) without detaching. No allocation or callback is needed.
A successful close invalidates the handle. Arbitrary/dangling handles are not
validated memory: never use one after close or context destruction.

## Ownership, GC, threading and transport

Userdata/callback code remain host-owned and live until detach or completion of
context destruction. Raw userdata is not scanned by GC: root any managed objects
separately, or preferably keep these controls purely native. Same owning-thread
rules as other context APIs apply. Audio callback guard blocks registry mutation,
nested stop-all and GC; guarded void context destruction is ignored. This is not
a mutex, worker cancellation or async signal handler API. Callbacks must not
block indefinitely or destroy their context.

Context destruction attempts every binding once after active channels, translates
and ignores terminal errors, deletes all native registrations and invalidates all
handles. Host resources must still exist until it returns. Hosts requiring stop
status must call stop explicitly before destruction. Unregistering before freeing
userdata avoids terminal callbacks into dead resources.

Registrations are process-local context state and are not graph-serialized.
Transporting the standard stopAllSounds function preserves its standard callback
identity and affects the destination context's own controls, never copies source
userdata. Normal native-function transport restrictions are unchanged.

## SDL player integration

[frontend/common/audio.c](../frontend/common/audio.c) registers after mixer/output
creation and before timeline notifications. Thus a StartSound followed by
StopSounds in the same frame cancels a prepared voice before first PCM acceptance.
A stop also clears pending failed-write PCM and queued data. Failed clear leaves
the producer paused/pending for explicit retry; no unregistration occurs.
The native worker can fill silence and resume; later StartSound events play.
Close detaches the binding, then stops/closes producer, mixer and SDL device before
window SDL_Quit. Context-channel semantics and resource-selective stop are unchanged.
`--mute` creates neither output nor binding. `--stats` adds successful sound_stops.

[03-stop-sounds.swf](../examples/player/animation_audio/03-stop-sounds.swf) cancels
frame-zero sound, stops the loop at frame30 and starts stereo PCM/ADPCM later.
It uses actual AVM1 opcode execution in interpreter/JIT, not a host-only trigger.
The ordinary two v0.582 demo files remain byte-identical. Full AS Sound, sprite
sound, exact DAC timing and drift-corrected A/V sync remain open.

## Verification limits

Strict C89 ABI, allocation-failure rollback/capacity and allocation-free stop/
detach, per-context isolation, GC/reentry/destruction guards, callback exceptions,
first-error/attempt-all order and actual interpreter/JIT pending-PCM cancellation
are regression-tested. The explicit frontend run uses the real SDL SDK and dummy
output under X11/Xvfb. This proves protocol execution, not physical audible output
or native Windows/Android/macOS acceptance. [Current status](ROADMAP_STATUS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:120](../include/flashvm/multimedia.h#L120) |
| `fvm_audio_stop_binding_close` | function | [include/flashvm/audio_stop.h:24](../include/flashvm/audio_stop.h#L24) |
| `fvm_audio_stop_binding_create` | function | [include/flashvm/audio_stop.h:19](../include/flashvm/audio_stop.h#L19) |
| `FVM_AUDIO_STOP_MAX_BINDINGS` | macro / define | [include/flashvm/audio_stop.h:9](../include/flashvm/audio_stop.h#L9) |
| `FVM_AudioStopBinding` | struct | [src/multimedia/audio/channel.hpp:15](../src/multimedia/audio/channel.hpp#L15) |
| `FVM_AudioStopCallback` | callback typedef | [include/flashvm/audio_stop.h:13](../include/flashvm/audio_stop.h#L13) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:184](../include/flashvm/swf_sound_output.h#L184) |
<!-- source-links:end -->
